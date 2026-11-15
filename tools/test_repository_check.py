#!/usr/bin/env python3
"""Exercise enforcement against temporary source trees, including negative cases."""
import sys
sys.dont_write_bytecode = True
import tempfile
import unittest
from pathlib import Path
import check_repository

class RepositoryChecks(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.previous = check_repository.ROOT
        check_repository.ROOT = Path(self.directory.name)

    def tearDown(self):
        check_repository.ROOT = self.previous
        self.directory.cleanup()

    def write(self, name, content=''):
        path = check_repository.ROOT / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    def test_first_party_extensions_are_rejected(self):
        for extension in check_repository.FORBIDDEN:
            self.write('include/example' + extension)
        self.assertEqual(len(check_repository.check()), len(check_repository.FORBIDDEN))

    def test_build_and_vendor_are_exempt(self):
        for directory in ('build', 'build-debug', 'external', 'vendor', 'third_party', 'include/glad', 'include/KHR'):
            self.write(directory + '/example.hpp')
        self.assertEqual(check_repository.check(), [])
        self.write('building/example.hpp')
        self.assertEqual(len(check_repository.check()), 1)

    def test_links_and_toc_are_checked(self):
        self.write('README.md', '# Example\n\n## Table of contents\n\n- [API](#api)\n\n## API\n\n[Header](include/api.h)\n')
        self.write('include/api.h')
        self.assertEqual(check_repository.check(), [])
        self.write('docs/broken.md', '# Broken\n\n[Missing](missing.md)\n[Anchor](../README.md#absent)\n')
        self.assertEqual(len(check_repository.check()), 3)

if __name__ == '__main__':
    unittest.main()
