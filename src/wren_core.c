#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wren_core.h"
#include "wren_value.h"

// Binds a native method named [name] (in Wren) implemented using C function
// [fn] to `ObjClass` [cls].
#define NATIVE(cls, name, fn) \
    { \
      int symbol = ensureSymbol(&vm->methods, name, strlen(name)); \
      cls->methods[symbol].type = METHOD_PRIMITIVE; \
      cls->methods[symbol].primitive = native_##fn; \
    }

// Binds a "fiber native" method named [name] (in Wren) implemented using C
// function [fn] to `ObjClass` [cls]. Unlike regular native methods, fiber
// natives have access to the fiber itself and can do lower-level stuff like
// pushing callframes.
#define FIBER_NATIVE(cls, name, fn) \
    { \
      int symbol = ensureSymbol(&vm->methods, name, strlen(name)); \
      cls->methods[symbol].type = METHOD_FIBER; \
      cls->methods[symbol].fiberPrimitive = native_##fn; \
    }

// Defines a native method whose C function name is [native]. This abstracts
// the actual type signature of a native function and makes it clear which C
// functions are intended to be invoked as natives.
#define DEF_NATIVE(native) \
    static Value native_##native(WrenVM* vm, Value* args)

// Defines a fiber native method whose C function name is [native].
#define DEF_FIBER_NATIVE(native) \
    static void native_##native(WrenVM* vm, ObjFiber* fiber, Value* args)

// TODO: Tune these.
// The initial (and minimum) capacity of a non-empty list object.
#define LIST_MIN_CAPACITY (16)

// The rate at which a list's capacity grows when the size exceeds the current
// capacity. The new capacity will be determined by *multiplying* the old
// capacity by this. Growing geometrically is necessary to ensure that adding
// to a list has O(1) amortized complexity.
#define LIST_GROW_FACTOR (2)

// This string literal is generated automatically from corelib.wren using
// make_corelib. Do not edit here.
const char* coreLibSource =
"class IO {\n"
"  static write(obj) {\n"
"    IO.write__native__(obj.toString)\n"
"    return obj\n"
"  }\n"
"}\n"
"\n"
"class List {\n"
"  toString {\n"
"    var result = \"[\"\n"
"    var i = 0\n"
"    // TODO: Use for loop.\n"
"    while (i < this.count) {\n"
"      if (i > 0) result = result + \", \"\n"
"      result = result + this[i].toString\n"
"      i = i + 1\n"
"    }\n"
"    result = result + \"]\"\n"
"    return result\n"
"  }\n"
"}\n";

DEF_NATIVE(bool_not)
{
  return BOOL_VAL(!AS_BOOL(args[0]));
}

DEF_NATIVE(bool_toString)
{
  // TODO: Intern these strings or something.
  if (AS_BOOL(args[0]))
  {
    return wrenNewString(vm, "true", 4);
  }
  else
  {
    return wrenNewString(vm, "false", 5);
  }
}

// The call instruction leading to this primitive has one argument for the
// receiver plus as many arguments as were passed. When we push the block onto
// the callstack, we again use as many arguments. That ensures that the result
// of evaluating the block goes into the slot that the caller of *this*
// primitive is expecting.
DEF_FIBER_NATIVE(fn_call0) { wrenCallFunction(fiber, args[0], 1); }
DEF_FIBER_NATIVE(fn_call1) { wrenCallFunction(fiber, args[0], 2); }
DEF_FIBER_NATIVE(fn_call2) { wrenCallFunction(fiber, args[0], 3); }
DEF_FIBER_NATIVE(fn_call3) { wrenCallFunction(fiber, args[0], 4); }
DEF_FIBER_NATIVE(fn_call4) { wrenCallFunction(fiber, args[0], 5); }
DEF_FIBER_NATIVE(fn_call5) { wrenCallFunction(fiber, args[0], 6); }
DEF_FIBER_NATIVE(fn_call6) { wrenCallFunction(fiber, args[0], 7); }
DEF_FIBER_NATIVE(fn_call7) { wrenCallFunction(fiber, args[0], 8); }
DEF_FIBER_NATIVE(fn_call8) { wrenCallFunction(fiber, args[0], 9); }
DEF_FIBER_NATIVE(fn_call9) { wrenCallFunction(fiber, args[0], 10); }
DEF_FIBER_NATIVE(fn_call10) { wrenCallFunction(fiber, args[0], 11); }
DEF_FIBER_NATIVE(fn_call11) { wrenCallFunction(fiber, args[0], 12); }
DEF_FIBER_NATIVE(fn_call12) { wrenCallFunction(fiber, args[0], 13); }
DEF_FIBER_NATIVE(fn_call13) { wrenCallFunction(fiber, args[0], 14); }
DEF_FIBER_NATIVE(fn_call14) { wrenCallFunction(fiber, args[0], 15); }
DEF_FIBER_NATIVE(fn_call15) { wrenCallFunction(fiber, args[0], 16); }
DEF_FIBER_NATIVE(fn_call16) { wrenCallFunction(fiber, args[0], 17); }

// Grows [list] if needed to ensure it can hold [count] elements.
static void ensureListCapacity(WrenVM* vm, ObjList* list, int count)
{
  if (list->capacity >= count) return;

  int capacity = list->capacity * LIST_GROW_FACTOR;
  if (capacity < LIST_MIN_CAPACITY) capacity = LIST_MIN_CAPACITY;

  list->capacity *= 2;
  list->elements = wrenReallocate(vm, list->elements,
      list->capacity * sizeof(Value), capacity * sizeof(Value));
  // TODO: Handle allocation failure.
  list->capacity = capacity;
}

// Validates that [index] is an integer within `[0, count)`. Also allows
// negative indices which map backwards from the end. Returns the valid positive
// index value, or -1 if the index wasn't valid (not a number, not an int, out
// of bounds).
static int validateIndex(Value index, int count)
{
  if (!IS_NUM(index)) return -1;

  double indexNum = AS_NUM(index);
  int intIndex = (int)indexNum;
  // Make sure the index is an integer.
  if (indexNum != intIndex) return -1;

  // Negative indices count from the end.
  if (indexNum < 0) indexNum = count + indexNum;

  // Check bounds.
  if (indexNum < 0 || indexNum >= count) return -1;

  return indexNum;
}

DEF_NATIVE(list_add)
{
  ObjList* list = AS_LIST(args[0]);

  ensureListCapacity(vm, list, list->count + 1);
  list->elements[list->count++] = args[1];
  return args[1];
}

DEF_NATIVE(list_clear)
{
  ObjList* list = AS_LIST(args[0]);
  wrenReallocate(vm, list->elements, 0, 0);
  list->elements = NULL;
  list->capacity = 0;
  list->count = 0;
  return NULL_VAL;
}

DEF_NATIVE(list_count)
{
  ObjList* list = AS_LIST(args[0]);
  return NUM_VAL(list->count);
}

DEF_NATIVE(list_insert)
{
  ObjList* list = AS_LIST(args[0]);

  // count + 1 here so you can "insert" at the very end.
  int index = validateIndex(args[2], list->count + 1);
  // TODO: Instead of returning null here, should signal an error explicitly
  // somehow.
  if (index == -1) return NULL_VAL;

  ensureListCapacity(vm, list, list->count + 1);

  // Shift items down.
  for (int i = list->count; i > index; i--)
  {
    list->elements[i] = list->elements[i - 1];
  }

  list->elements[index] = args[1];
  list->count++;
  return args[1];
}

DEF_NATIVE(list_removeAt)
{
  ObjList* list = AS_LIST(args[0]);
  int index = validateIndex(args[1], list->count);
  // TODO: Instead of returning null here, should signal an error explicitly
  // somehow.
  if (index == -1) return NULL_VAL;

  Value removed = list->elements[index];

  // Shift items up.
  for (int i = index; i < list->count - 1; i++)
  {
    list->elements[i] = list->elements[i + 1];
  }

  // If we have too much excess capacity, shrink it.
  if (list->capacity / LIST_GROW_FACTOR >= list->count)
  {
    wrenReallocate(vm, list->elements, sizeof(Value) * list->capacity,
                   sizeof(Value) * (list->capacity / LIST_GROW_FACTOR));
    list->capacity /= LIST_GROW_FACTOR;
  }

  list->count--;
  return removed;
}

DEF_NATIVE(list_subscript)
{
  ObjList* list = AS_LIST(args[0]);

  int index = validateIndex(args[1], list->count);
  // TODO: Instead of returning null here, should signal an error explicitly
  // somehow.
  if (index == -1) return NULL_VAL;

  return list->elements[index];
}

DEF_NATIVE(list_subscriptSetter)
{
  ObjList* list = AS_LIST(args[0]);

  int index = validateIndex(args[1], list->count);
  // TODO: Instead of returning null here, should signal an error explicitly
  // somehow.
  if (index == -1) return NULL_VAL;

  list->elements[index] = args[2];
  return args[2];
}

DEF_NATIVE(null_toString)
{
  // TODO: Intern this string or something.
  return wrenNewString(vm, "null", 4);
}

DEF_NATIVE(num_abs)
{
  return NUM_VAL(fabs(AS_NUM(args[0])));
}

DEF_NATIVE(num_toString)
{
  // TODO: What size should this be?
  char temp[100];
  sprintf(temp, "%.14g", AS_NUM(args[0]));
  return (Value)wrenNewString(vm, temp, strlen(temp));
}

DEF_NATIVE(num_negate)
{
  return NUM_VAL(-AS_NUM(args[0]));
}

DEF_NATIVE(num_minus)
{
  // TODO: Handle unsupported operand types better.
  if (!IS_NUM(args[1])) return NULL_VAL;
  return NUM_VAL(AS_NUM(args[0]) - AS_NUM(args[1]));
}

DEF_NATIVE(num_plus)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  // TODO: Handle coercion to string if RHS is a string.
  return NUM_VAL(AS_NUM(args[0]) + AS_NUM(args[1]));
}

DEF_NATIVE(num_multiply)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return NUM_VAL(AS_NUM(args[0]) * AS_NUM(args[1]));
}

DEF_NATIVE(num_divide)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return NUM_VAL(AS_NUM(args[0]) / AS_NUM(args[1]));
}

DEF_NATIVE(num_mod)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return NUM_VAL(fmod(AS_NUM(args[0]), AS_NUM(args[1])));
}

DEF_NATIVE(num_lt)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return BOOL_VAL(AS_NUM(args[0]) < AS_NUM(args[1]));
}

DEF_NATIVE(num_gt)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return BOOL_VAL(AS_NUM(args[0]) > AS_NUM(args[1]));
}

DEF_NATIVE(num_lte)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return BOOL_VAL(AS_NUM(args[0]) <= AS_NUM(args[1]));
}

DEF_NATIVE(num_gte)
{
  if (!IS_NUM(args[1])) return NULL_VAL;
  return BOOL_VAL(AS_NUM(args[0]) >= AS_NUM(args[1]));
}

DEF_NATIVE(num_eqeq)
{
  if (!IS_NUM(args[1])) return FALSE_VAL;
  return BOOL_VAL(AS_NUM(args[0]) == AS_NUM(args[1]));
}

DEF_NATIVE(num_bangeq)
{
  if (!IS_NUM(args[1])) return TRUE_VAL;
  return BOOL_VAL(AS_NUM(args[0]) != AS_NUM(args[1]));
}

DEF_NATIVE(num_bitwiseNot)
{
  // Bitwise operators always work on 32-bit unsigned ints.
  uint32_t value = (uint32_t)AS_NUM(args[0]);
  return NUM_VAL(~value);
}

DEF_NATIVE(object_eqeq)
{
  return BOOL_VAL(wrenValuesEqual(args[0], args[1]));
}

DEF_NATIVE(object_bangeq)
{
  return BOOL_VAL(!wrenValuesEqual(args[0], args[1]));
}

DEF_NATIVE(object_new)
{
  // This is the default argument-less constructor that all objects inherit.
  // It just returns "this".
  return args[0];
}

DEF_NATIVE(object_toString)
{
  return wrenNewString(vm, "<object>", 8);
}

DEF_NATIVE(object_type)
{
  return OBJ_VAL(wrenGetClass(vm, args[0]));
}

DEF_NATIVE(string_contains)
{
  const char* string = AS_CSTRING(args[0]);
  // TODO: Check type of arg first!
  const char* search = AS_CSTRING(args[1]);

  // Corner case, the empty string contains the empty string.
  if (strlen(string) == 0 && strlen(search) == 0) return TRUE_VAL;

  return BOOL_VAL(strstr(string, search) != NULL);
}

DEF_NATIVE(string_count)
{
  double count = strlen(AS_CSTRING(args[0]));
  return NUM_VAL(count);
}

DEF_NATIVE(string_toString)
{
  return args[0];
}

DEF_NATIVE(string_plus)
{
  if (!IS_STRING(args[1])) return NULL_VAL;
  // TODO: Handle coercion to string of RHS.

  const char* left = AS_CSTRING(args[0]);
  const char* right = AS_CSTRING(args[1]);

  size_t leftLength = strlen(left);
  size_t rightLength = strlen(right);

  Value value = wrenNewString(vm, NULL, leftLength + rightLength);
  ObjString* string = AS_STRING(value);
  strcpy(string->value, left);
  strcpy(string->value + leftLength, right);
  string->value[leftLength + rightLength] = '\0';

  return value;
}

DEF_NATIVE(string_eqeq)
{
  if (!IS_STRING(args[1])) return FALSE_VAL;
  const char* a = AS_CSTRING(args[0]);
  const char* b = AS_CSTRING(args[1]);
  return BOOL_VAL(strcmp(a, b) == 0);
}

DEF_NATIVE(string_bangeq)
{
  if (!IS_STRING(args[1])) return TRUE_VAL;
  const char* a = AS_CSTRING(args[0]);
  const char* b = AS_CSTRING(args[1]);
  return BOOL_VAL(strcmp(a, b) != 0);
}

DEF_NATIVE(string_subscript)
{
  // TODO: Instead of returning null here, all of these failure cases should
  // signal an error explicitly somehow.
  if (!IS_NUM(args[1])) return NULL_VAL;

  double indexNum = AS_NUM(args[1]);
  int index = (int)indexNum;
  // Make sure the index is an integer.
  if (indexNum != index) return NULL_VAL;

  ObjString* string = AS_STRING(args[0]);

  // Negative indices count from the end.
  // TODO: Strings should cache their length.
  int length = (int)strlen(string->value);
  if (index < 0) index = length + index;

  // Check bounds.
  if (index < 0 || index >= length) return NULL_VAL;

  // The result is a one-character string.
  // TODO: Handle UTF-8.
  Value value = wrenNewString(vm, NULL, 2);
  ObjString* result = AS_STRING(value);
  result->value[0] = AS_CSTRING(args[0])[index];
  result->value[1] = '\0';
  return value;
}

DEF_NATIVE(io_write)
{
  // TODO: Should call toString on its argument.
  wrenPrintValue(args[1]);
  printf("\n");
  return args[1];
}

DEF_NATIVE(os_clock)
{
  double time = (double)clock() / CLOCKS_PER_SEC;
  return NUM_VAL(time);
}

static ObjClass* defineClass(WrenVM* vm, const char* name)
{
  ObjClass* classObj = wrenNewClass(vm, vm->objectClass, 0);
  int symbol = addSymbol(&vm->globalSymbols, name, strlen(name));
  vm->globals[symbol] = OBJ_VAL(classObj);
  return classObj;
}

void wrenInitializeCore(WrenVM* vm)
{
  // Define the root Object class. This has to be done a little specially
  // because it has no superclass and an unusual metaclass (Class).
  vm->objectClass = wrenNewSingleClass(vm, 0);
  int objectSymbol = addSymbol(&vm->globalSymbols, "Object", strlen("Object"));
  vm->globals[objectSymbol] = OBJ_VAL(vm->objectClass);

  NATIVE(vm->objectClass, "== ", object_eqeq);
  NATIVE(vm->objectClass, "!= ", object_bangeq);
  NATIVE(vm->objectClass, "new", object_new);
  NATIVE(vm->objectClass, "toString", object_toString);
  NATIVE(vm->objectClass, "type", object_type);

  // Now we can define Class, which is a subclass of Object, but Object's
  // metaclass.
  vm->classClass = wrenNewSingleClass(vm, 0);
  int classSymbol = addSymbol(&vm->globalSymbols, "Class", strlen("Class"));
  vm->globals[classSymbol] = OBJ_VAL(vm->classClass);

  // Now that Object and Class are defined, we can wire them up to each other.
  wrenBindSuperclass(vm->classClass, vm->objectClass);
  vm->objectClass->metaclass = vm->classClass;
  vm->classClass->metaclass = vm->classClass;

  // The core class diagram ends up looking like this, where single lines point
  // to a class's superclass, and double lines point to its metaclass:
  //
  //             __________        /====\
  //            /          \      //    \\
  //           v            \     v      \\
  //     .---------.   .--------------.  //
  //     | Object  |==>|    Class     |==/
  //     '---------'   '--------------'
  //          ^               ^
  //          |               |
  //     .---------.   .--------------.   \
  //     |  Base   |==>|  Base.type   |    |
  //     '---------'   '--------------'    |
  //          ^               ^            | Hypothetical example classes
  //          |               |            |
  //     .---------.   .--------------.    |
  //     | Derived |==>| Derived.type |    |
  //     '---------'   '--------------'    /

  // The rest of the classes can not be defined normally.
  vm->boolClass = defineClass(vm, "Bool");
  NATIVE(vm->boolClass, "toString", bool_toString);
  NATIVE(vm->boolClass, "!", bool_not);

  vm->fiberClass = defineClass(vm, "Fiber");
  
  vm->fnClass = defineClass(vm, "Function");
  FIBER_NATIVE(vm->fnClass, "call", fn_call0);
  FIBER_NATIVE(vm->fnClass, "call ", fn_call1);
  FIBER_NATIVE(vm->fnClass, "call  ", fn_call2);
  FIBER_NATIVE(vm->fnClass, "call   ", fn_call3);
  FIBER_NATIVE(vm->fnClass, "call    ", fn_call4);
  FIBER_NATIVE(vm->fnClass, "call     ", fn_call5);
  FIBER_NATIVE(vm->fnClass, "call      ", fn_call6);
  FIBER_NATIVE(vm->fnClass, "call       ", fn_call7);
  FIBER_NATIVE(vm->fnClass, "call        ", fn_call8);
  FIBER_NATIVE(vm->fnClass, "call         ", fn_call9);
  FIBER_NATIVE(vm->fnClass, "call          ", fn_call10);
  FIBER_NATIVE(vm->fnClass, "call           ", fn_call11);
  FIBER_NATIVE(vm->fnClass, "call            ", fn_call12);
  FIBER_NATIVE(vm->fnClass, "call             ", fn_call13);
  FIBER_NATIVE(vm->fnClass, "call              ", fn_call14);
  FIBER_NATIVE(vm->fnClass, "call               ", fn_call15);
  FIBER_NATIVE(vm->fnClass, "call                ", fn_call16);

  vm->nullClass = defineClass(vm, "Null");
  NATIVE(vm->nullClass, "toString", null_toString);

  vm->numClass = defineClass(vm, "Num");
  NATIVE(vm->numClass, "abs", num_abs);
  NATIVE(vm->numClass, "toString", num_toString)
  NATIVE(vm->numClass, "-", num_negate);
  NATIVE(vm->numClass, "- ", num_minus);
  NATIVE(vm->numClass, "+ ", num_plus);
  NATIVE(vm->numClass, "* ", num_multiply);
  NATIVE(vm->numClass, "/ ", num_divide);
  NATIVE(vm->numClass, "% ", num_mod);
  NATIVE(vm->numClass, "< ", num_lt);
  NATIVE(vm->numClass, "> ", num_gt);
  NATIVE(vm->numClass, "<= ", num_lte);
  NATIVE(vm->numClass, ">= ", num_gte);
  NATIVE(vm->numClass, "~", num_bitwiseNot);

  // These are defined just so that 0 and -0 are equal, which is specified by
  // IEEE 754 even though they have different bit representations.
  NATIVE(vm->numClass, "== ", num_eqeq);
  NATIVE(vm->numClass, "!= ", num_bangeq);

  vm->stringClass = defineClass(vm, "String");
  NATIVE(vm->stringClass, "contains ", string_contains);
  NATIVE(vm->stringClass, "count", string_count);
  NATIVE(vm->stringClass, "toString", string_toString)
  NATIVE(vm->stringClass, "+ ", string_plus);
  NATIVE(vm->stringClass, "== ", string_eqeq);
  NATIVE(vm->stringClass, "!= ", string_bangeq);
  NATIVE(vm->stringClass, "[ ]", string_subscript);

  ObjClass* osClass = defineClass(vm, "OS");
  NATIVE(osClass->metaclass, "clock", os_clock);

  wrenInterpret(vm, coreLibSource);

  vm->listClass = AS_CLASS(findGlobal(vm, "List"));
  NATIVE(vm->listClass, "add ", list_add);
  NATIVE(vm->listClass, "clear", list_clear);
  NATIVE(vm->listClass, "count", list_count);
  NATIVE(vm->listClass, "insert  ", list_insert);
  NATIVE(vm->listClass, "removeAt ", list_removeAt);
  NATIVE(vm->listClass, "[ ]", list_subscript);
  NATIVE(vm->listClass, "[ ]=", list_subscriptSetter);

  ObjClass* ioClass = AS_CLASS(findGlobal(vm, "IO"));
  NATIVE(ioClass->metaclass, "write__native__ ", io_write);
}