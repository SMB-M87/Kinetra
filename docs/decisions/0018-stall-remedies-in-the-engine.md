# 0018: Stall detection and remedies in the engine, and i4sim on the passing coordinator

Status: accepted. Amends [0017](0017-passing-coordinator.md): i4sim runs its passing on `PassingCoordinator`, and stall detection, a right-of-way fallback and making way become engine mechanisms beside it.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

[ADR 0017](0017-passing-coordinator.md) moved the passing lifecycle into the engine for Demo and Dotio, and left two questions open:

- whether i4sim, with its own factory coordinator, moves onto it;
- whether making way and a temporary motion fallback belong in it.

Kinetra-Lab answered the second with measurements in i4sim ([design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-27-stall-remedies-in-the-engine.md)). Of the long stalls left once MAKE WAY is on, most under Reciprocal are a mover near its goal pushed back by a neighbor driving through its spot. A right-of-way window of 480 ticks for that mover cuts waiting by 27% without a contact. i4sim's coordinator is the lifecycle 0017 extracted. Its remaining differences change no plan outcome in i4sim's pressure scenarios, only a few positions: the retry delay after a goal change, the guard query margin, and the heading a mover forms its next plan from.

On 2026-11-27 the user chose four things:

- i4sim moves onto the coordinator, and its lines with PASSING on are recorded again.
- Stall detection, the right-of-way fallback and making way also become engine mechanisms, so every application can use them.
- They are classes of their own beside the coordinator, not part of it.

## Decision

- **i4sim uses `PassingCoordinator`.** Its passing component maps the factory's movers to `PassingMoverState`, checks lanes against the floor and its traffic zones, and turns each target into a traffic control. It derives its finer end reasons (stopped, held, traffic control, goal changed, disabled) from the mover states. The coordinator gets no factory option.
- **Stall remedies are engine mechanisms beside the coordinator**, each with its own per-step contract and events, in `Kinetra::Simulation`:
  - a `StallDetector` that observes progress the application measures and diagnoses an episode, including a mover pushed back by an approaching neighbor;
  - a `RightOfWay` fallback that lets such a mover stop cooperating for a fixed window;
  - a `MakeWay` lifecycle that asks one standing neighbor to take a refuge the application offers, and brings it back.
- **The application keeps its policy**: what progress means, which movers may yield, refuge candidates and the world check of a route, and what a yielding mover keeps (i4sim's product, queue slot and parking reservation).

Rejected:

- *Keep the remedies in i4sim*, the agent's recommendation: it would stay the smallest engine API, but Demo and Dotio could not use them without a copy.
- *One coordinator for passing and remedies*: it enlarges 0017's contract and couples mechanisms that an application may want separately.
- *Engine options so i4sim's recorded lines stay exact*: three options that serve one application's history.

## Consequences

- One passing lifecycle runs in all three applications. i4sim's pressure lines with PASSING on were recorded again once, with every plan outcome, arrival and contact count unchanged.
- The engine gains three public types with their own tests; each lands in its own pull request, with i4sim's use of it in Kinetra-Lab.
- Demo and Dotio can adopt detection and remedies later, without a copy.
- A consumer that needs another remedy rule asks for an extension of these types.
- The priority rule of passing stays as 0017 fixed it.
