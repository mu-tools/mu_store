# mu_store - Lightweight Fixed-Memory Data Structures

The `mu_store` project provides a collection of lightweight data structure modules designed for embedded C programming environments. The key principle is the use of pre-allocated, user-provided memory for the data storage, avoiding dynamic heap allocation within the modules themselves. This makes them suitable for systems with strict memory constraints or real-time requirements where heap unpredictable behavior is undesirable.

Each module focuses on a specific data structure or memory management pattern.

## Modules

Here are the modules included in the `mu_store` project:

* **`mu_store`**:
    * **Description:** Provides core definitions, common types (like error codes and insertion policies), and potentially foundational utilities used by other modules in the `mu_store` collection.
    * **Documentation:** [mu_store/README.md](mu_store/README.md)

* **`mu_pool`**:
    * **Description:** Implements a resource pool manager for homogeneous objects of a fixed size. Allows allocating and freeing items from a pre-defined block of memory.
    * **Documentation:** [mu_pool/README.md](mu_pool/README.md)

* **`mu_queue`**:
    * **Description:** Provides a queue data structure implementation. (Details in its specific documentation).
    * **Documentation:** [mu_queue/README.md](mu_queue/README.md)

* **`mu_spsc`**:
    * **Description:** Implements a data structure for thread-safe communication between a single producer and a single consumer, typically a ring buffer or queue. (Details in its specific documentation).
    * **Documentation:** [mu_spsc/README.md](mu_spsc/README.md)

* **`mu_vec`**:
    * **Description:** A generic vector (dynamic array) implementation for storing items of arbitrary size in a contiguous, user-provided memory buffer.
    * **Documentation:** [mu_vec/README.md](mu_vec/README.md)

* **`mu_pvec`**:
    * **Description:** A specialized vector (dynamic array) implementation optimized for storing `void*` pointers in a contiguous, user-provided array of pointers.
    * **Documentation:** [mu_pvec/README.md](mu_pvec/README.md)

## Getting Started

To use these modules in your project:

1.  Include the desired module's header file (`#include "<module_name>.h"`).
2.  In your application code, declare and allocate the necessary memory buffer(s) for the data structure's storage (e.g., an array of structs for `mu_vec`, an array of `void*` for `mu_pvec`, a buffer for `mu_pool`).
3.  Initialize the data structure instance, passing the allocated memory and capacity.
4.  Use the module's API functions to interact with the data structure.

Refer to each module's specific `README.md` for detailed API documentation and usage examples.