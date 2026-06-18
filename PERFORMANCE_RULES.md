# Performance Rules

## Allocation Rules
- Avoid frequent dynamic memory allocations (e.g., with `new` or `std::vector::push_back`) in performance-critical code paths. Pre-allocate memory whenever possible.

## Threading Rules
- Never perform long-running computations on the main UI thread. Offload them to a worker thread.
- Use signals and slots to communicate between the worker thread and the main thread.

## UI Responsiveness Rules
- Ensure that event handlers (like slider moved) execute quickly.
- For expensive operations, use a debouncing or throttling mechanism to limit how often they are executed.

## What Must Never Be Done in Hot Paths
- File I/O.
- Complex scene graph manipulations.
- Shader compilation.
- Blocking network requests.
