// #include <chrono>
// #include <iostream>
// #include <thread>

// #include "memoryspace.hpp"

// #include "compiler.hpp"
// #include "memory.hpp"
// #include "vm.hpp"

// void markCompilerRoots();
// static void markObject(Obj* object, VM* vm);
// static void markValue(Value value, VM* vm);
// void markCompilerRoots();
// void collectGarbage();
// void freeObject(Obj* object);
// void freeObjects();

// void freeObject(Obj* object)
// {
//   // #ifdef DEBUG_LOG_GC
//   printf("%p free type %d \n", (void*)object, object->type);
//   // #endif
//   switch (object->type) {
//     case OBJ_BOUND_METHOD:
//       FREE<ObjBoundMethod>(object);
//       break;
//     case OBJ_CLASS: {
//       auto klass = (ObjClass*)object;
//       klass->methods.freeTable();
//       FREE<ObjClass>(object);
//       break;
//     }
//     case OBJ_INSTANCE: {
//       auto instance = (ObjInstance*)object;
//       instance->fields.freeTable();
//       FREE<ObjInstance>(object);
//       break;
//     }
//     case OBJ_CLOSURE: {
//       auto closure = (ObjClosure*)object;
//       FREE_ARRAY<ObjUpvalue*>(closure->upvalues, closure->upvalueCount);
//       FREE<ObjClosure>(object);
//       break;
//     }
//     case OBJ_FUNCTION: {
//       auto function = (ObjFunction*)object;
//       function->chunk.freeChunk();
//       FREE<ObjFunction>(object);
//       break;
//     }
//     case OBJ_NATIVE:
//       FREE<ObjNative>(object);
//       break;
//     case OBJ_STRING: {
//       auto string = (ObjString*)object;
//       FREE_ARRAY<char>(string->chars, string->length + 1);
//       FREE<ObjString>(object);
//       break;
//     }
//     case OBJ_UPVALUE:
//       FREE<ObjUpvalue>(object);
//       break;
//     case OBJ_LIST: {
//       ObjList* list = (ObjList*)object;
//       FREE_ARRAY<Value>(list->items, list->count);
//       FREE<ObjList>(object);
//       break;
//     }
//     case OBJ_FUTURE: {
//       auto futureObj = (ObjFuture*)object;
//       // TODO: call dispatcher to free lock resources if any
//       FREE<ObjFuture>(futureObj);
//       break;
//     }
//     case OBJ_MUTEX: {
//       auto mutexObj = (ObjMutex*)object;
//       // TODO: call lockmanager to free resources if any
//       FREE<ObjMutex>(mutexObj);
//       break;
//     }
//     case OBJ_CHANNEL: {
//       auto channelObj = (ObjChannel*)object;
//       FREE<ObjChannel>(channelObj);
//       break;
//     }
//   }
// }

// // static inline int GROW_CAPACITY(int capacity)
// // {
// //   return (capacity) < 8 ? 8 : (capacity)*2;
// // }

// static void markArray(ValueArray* array, VM* vm)
// {
//   for (int i = 0; i < array->count; i++) {
//     markValue(array->values[i], vm);
//   }
// }

// static void blackenObject(Obj* object, VM* vm)
// {
// #ifdef DEBUG_LOG_GC
//   printf("%p blacken ", (void*)object);
//   printValue(OBJ_VAL(object));
//   printf("\n");
// #endif
//   switch (object->type) {
//     case OBJ_BOUND_METHOD: {
//       auto bound = (ObjBoundMethod*)object;
//       markValue(bound->receiver, vm);
//       markObject((Obj*)bound->method, vm);
//       break;
//     }
//     case OBJ_CLASS: {
//       auto klass = (ObjClass*)object;
//       markObject((Obj*)klass->name, vm);
//       klass->methods.markTable(vm);
//       break;
//     }
//     case OBJ_INSTANCE: {
//       auto instance = (ObjInstance*)object;
//       markObject((Obj*)instance->klass, vm);
//       instance->fields.markTable(vm);
//       break;
//     }
//     case OBJ_CLOSURE: {
//       auto closure = (ObjClosure*)object;
//       markObject((Obj*)closure->function, vm);
//       for (int i = 0; i < closure->upvalueCount; i++) {
//         markObject((Obj*)closure->upvalues[i], vm);
//       }
//       break;
//     }
//     case OBJ_FUNCTION: {
//       auto function = (ObjFunction*)object;
//       markObject((Obj*)function->name, vm);
//       markArray(&function->chunk.constants, vm);
//       break;
//     }
//     case OBJ_UPVALUE:
//       markValue(((ObjUpvalue*)object)->closed, vm);
//       break;
//     case OBJ_LIST: {
//       ObjList* list = (ObjList*)object;
//       for (int i = 0; i < list->count; i++) {
//         markValue(list->items[i], vm);
//       }
//       break;
//     }
//     case OBJ_FUTURE:
//     case OBJ_MUTEX:
//     case OBJ_CHANNEL:
//     case OBJ_NATIVE:
//     case OBJ_STRING:
//       break;
//   }
// }

// static void traceReferences(VM* vm)
// {
//   while (vm->grayCount > 0) {
//     auto object = vm->grayStack[--vm->grayCount];
//     blackenObject(object, vm);
//   }
// }

// static void markObject(Obj* object, VM* vm)
// {
//   if (object == NULL)
//     return;
//   if (object->isMarked)
//     return;
// #ifdef DEBUG_LOG_GC
//   printf("%p mark ", (void*)object);
//   printValue(OBJ_VAL(object));
//   printf("\n");
// #endif

//   object->isMarked = true;
//   if (vm->grayCapacity < vm->grayCount + 1) {
//     vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
//     vm->grayStack =
//         (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);
//     if (vm->grayStack == NULL) {
//       exit(1);
//     }
//   }
//   vm->grayStack[vm->grayCount++] = object;
// }

// static void markValue(Value value, VM* vm)
// {
//   if (IS_OBJ(value))
//     markObject(AS_OBJ(value), vm);
// }

// static void markRoots(VM* vm)
// {
//   printf("Start Marky Marky \n");
//   for (auto slot = vm->stack; slot < vm->stackTop; slot++) {
//     markValue(*slot, vm);
//   }
//   // TODO: fix this with VM
//   for (int i = 0; i < vm->frameCount; i++) {
//     markObject((Obj*)vm->frames[i].closure, vm);
//   }
//   for (auto upvalue = vm->openUpvalues; upvalue != NULL;
//        upvalue = upvalue->next) {
//     markObject((Obj*)upvalue, vm);
//   }
//   vm->globals.markTable(vm);
//   markCompilerRoots();
//   markObject((Obj*)vm->initString, vm);

//   printf("Done Marky Marky \n");
// }

// // Memory Generation
// void MemoryGeneration::initMG(Generation gen, MemorySpace* ms)
// {
//   this->head = NULL;
//   this->gen = gen;
//   this->bytesAllocated = 0;
//   this->ms = ms;
//   switch (this->gen) {
//     case Generation::NURSERY:
//       this->nextSweep = 1024 * 1024;
//       break;
//     case Generation::SURVIVOR:
//       this->nextSweep = 1024 * 1024 * 16;
//       break;
//     case Generation::TENURED:
//       this->nextSweep = 1024 * 1024 * 128;
//       break;
//     default:
//       break;
//   }
// }

// void MemoryGeneration::sweep()
// {
//   printf("Sweepy Sweepy time \n");
//   auto object = this->head;
//   Obj* previous = NULL;

//   while (object != NULL) {
//     if (object->isMarked) {
//       object->isMarked = false;
//       previous = object;
//       object = object->next;
//     } else {
//       auto unreached = object;
//       object = object->next;
//       if (previous != NULL) {
//         previous->next = object;
//       } else {
//         this->head = object;
//       }

//       freeObject(unreached);
//     }
//   }
//   printf("Done Sweepy Sweepy time \n");
// }

// void MemoryGeneration::addObject(Obj* newNode)
// {
//   if (this->head == NULL) {
//     this->head == newNode;
//   }
//   newNode->next = this->head;
//   this->head = newNode;
// }

// bool MemoryGeneration::updateStorage(int size)
// {
//   // printf("currently allocated: %d, new size: %d, next sweep: %d \n",
//   //        this->bytesAllocated,
//   //        size,
//   //        this->nextSweep);
//   this->bytesAllocated += size;
//   if (size > 0) {
//     if (this->bytesAllocated > this->nextSweep) {
//       if (this->gen == Generation::NURSERY) {
//         this->ms->waitMarkingThread();
//         this->ms->forcedMark();
//       }
//       this->sweep();
//       if (this->gen == Generation::NURSERY) {
//         this->ms->resumeMarkingThread();
//       }
//       this->increaseCapacity();
//       return true;
//     }
//   }
//   return false;
// }

// inline int MemoryGeneration::increaseCapacity()
// {
//   switch (this->gen) {
//     case Generation::NURSERY:
//       this->nextSweep =
//           static_cast<int>(static_cast<float>(this->nextSweep) * 1.05);
//     case Generation::SURVIVOR:
//       this->nextSweep =
//           static_cast<int>(static_cast<float>(this->nextSweep) * 2.2);
//     case Generation::TENURED:
//       this->nextSweep =
//           static_cast<int>(static_cast<float>(this->nextSweep) * 3.3);
//       break;
//     default:
//       break;
//   }
// }

// bool MemoryGeneration::resetStorage()
// {
//   this->bytesAllocated = 0;
// }

// // Memory Space

// bool MemorySpace::moveGenerations(MemoryGeneration& A, MemoryGeneration& B)
// {
//   auto res = B.updateStorage(A.bytesAllocated);
//   auto temp = B.head;
//   if (temp == NULL) {
//     B.head = A.head;
//     return res;
//   }
//   while (temp->next != NULL) {
//     temp = temp->next;
//   }
//   temp->next = A.head;
//   A.head = NULL;
//   return res;
// }

// void MemorySpace::doMark()
// {
//   while (true) {
//     if (this->startMarking) {
//       // std::cout << "Hello World " << std::endl;
//       markRoots(this->vm);
//       std::this_thread::sleep_for(std::chrono::milliseconds(5));
//       // std::cout << "Marking Stack" << std::endl;
//     }
//   }
// }

// void MemorySpace::initMS(VM* vm)
// {
//   this->nursery.initMG(Generation::NURSERY, this);
//   this->survivor.initMG(Generation::SURVIVOR, this);
//   this->tenured.initMG(Generation::TENURED, this);
//   this->vm = vm;
// }

// void MemorySpace::startMarkingThread()
// {
//   printf("Starting marking thread \n");
//   this->startMarking = true;
//   memoryThread.emplace_back([this] { this->doMark(); });
// }

// void MemorySpace::checkForTenuredGeneration() {}

// void MemorySpace::updateNurseryStorage(int size)
// {
//   auto didNurserySweep = this->nursery.updateStorage(size);
//   if (didNurserySweep) {
//     auto didSurvivorsWeep =
//         this->moveGenerations(this->nursery, this->survivor);
//     if (didSurvivorsWeep) {
//       // this->checkForTenuredGeneration();
//     }
//   }
// }

// void MemorySpace::addObjectToNursery(Obj* newNode)
// {
//   this->nursery.addObject(newNode);
// }

// void MemorySpace::forcedMark()
// {
//   // this->doMark();
//   markRoots(this->vm);
// }

// void MemorySpace::resumeMarkingThread()
// {
//   // printf("Resuming marking thread \n");
//   // this->startMarking = true;
// }

// void MemorySpace::waitMarkingThread()
// {
//   // printf("Pausing marking thread \n");
//   this->startMarking = false;
// }

// void MemorySpace::freeMarkingThread()
// {
//   // printf("Freeing marking thread \n");
//   this->startMarking = false;
// }

// MemorySpace::MemorySpace()
// {
//   // this->initMS(vm);
// }
