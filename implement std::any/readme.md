## Exercise 1: Type-Erased Storage with Small-Object Optimization (SOO)

### Español

#### Descripción del Problema
Este problema de programación en C++ evalúa la capacidad de construir un contenedor seguro de tipos (*type-safe*) y eficiente en memoria que pueda almacenar objetos de cualquier tipo. Es un ejercicio representativo de las entrevistas de desarrollo de sistemas en empresas de trading cuantitativo (*Quant Trading*), donde el dominio de la disposición de memoria (*memory layout*), las estrategias de asignación y la metaprogramación en C++ son fundamentales.

El reto principal radica en equilibrar dos objetivos contrapuestos:
1. **Tipos pequeños:** Almacenar tipos pequeños (como enteros o punteros) directamente en la pila (*stack*) para evitar la sobrecarga de asignación de memoria dinámica.
2. **Tipos grandes:** Delegar los tipos de mayor tamaño al montón (*heap*).

Además, se debe implementar una verificación de tipos en tiempo de ejecución (*runtime type checking*) para lanzar una excepción al intentar recuperar un valor con el tipo incorrecto, garantizando al mismo tiempo que la limpieza de recursos ocurra automáticamente mediante RAII al destruir el contenedor. El presupuesto estricto de tamaño exige un diseño cuidadoso de la representación interna de la estructura de datos.

#### Conceptos Clave
* **Type Erasure** e Información de Tipos en Tiempo de Ejecución (RTTI).
* Compromisos y balance en la **Optimización de Objetos Pequeños (SOO)**.
* Disposición de memoria (*memory layout*) y alineación en C++.
* **RAII** y gestión automática de recursos.
* Punteros a funciones o *virtual dispatch* para operaciones agnósticas al tipo.

---

### English

#### Problem Description
This C++ coding challenge tests your ability to build a type-safe, memory-efficient container capable of holding objects of arbitrary types. It is representative of systems-level interviews at quantitative trading firms, where understanding memory layout, allocation strategy, and C++ metaprogramming are crucial.

The core challenge is balancing two competing goals:
1. **Small types:** Storing small types (like integers or raw pointers) directly on the stack to avoid heap allocation overhead.
2. **Large types:** Delegating larger types to the heap.

Additionally, you must implement runtime type checking so that retrieving a value as the wrong type triggers an exception, while ensuring automatic cleanup when the container is destroyed via RAII. The strict size budget forces careful consideration of the internal data structure representation.

#### Key Concepts
* **Type erasure** and Runtime Type Information (RTTI).
* **Small-Object Optimization (SOO)** trade-offs.
* Memory layout and alignment in C++.
* **RAII** and resource cleanup.
* Function pointers or virtual dispatch for type-agnostic operations.
