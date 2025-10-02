# Stupid OOP things

oop is going to make me go insane.

## Class hierarchy and pointer ownership and lifetimes.

I want to generate subclasses from a parent class. This parent class owns the
subclasses (stored in a vector). The parent class has a method that generates
the subclasses. This method also returns a pointer to the parent class'
subclasses. Should the parent class' vector contain heap allocated pointers to
the subclasses (which are also returned), or should the vector contain the
actual subclass, with stack allocated pointers to the subclass returned?

The problem with the second method is that if we wanted to increase the amount
of subclasses stored, this would involve resizing the vector. If we returned the
pointer to the original vector's variable, then after resizing the pointer
would become invalid. Thus we need to use heap allocated subclasses, which the
vector stores the pointer to.

If I use not-smart heap allocated pointers, then i will have a memory leak when
the parent goes out of scope, as the raw pointers won't have their destructor's
called.

# access specifiers and inheritance

## Access specifiers

`protected` member variables - Allows extension of features upon this base class
as derived classes can also access these (for `public` and `protected` inheritance)

`private` member variables - Does **NOT** allow extension of features upon this
base class for derived classes (For all types of inheritance).

`public` member variables - Do not do unless they are `const`.

## Inheritance

`protected` inheritance - All `public` member variables become `protected`.

`private` inheritance - All member vars and functions are only accessible
inside the class.
