///////////////////////
// Table of Contents //
///////////////////////
Commenting
Brackets
Indentation
Spacing
Line Length
Functions
Annotations
Pointers
Misc

////////////////
// Commenting //
////////////////

// ok
/* not ok */

// ok
//not ok

//////////////
// Brackets //
//////////////

// These 2 styles are ok, but they cannot both exist in the same file
void fileAFunction()
{
}
void fileBFunction() {
}

/////////////////
// Indentation //
/////////////////
// Tab = 2 spaces
void Foo()
{
  Bar();
}

/////////////
// Spacing //
/////////////
// parenthesis have an inner padding of a space
int Function( int a, int* values, int count )
{
  int sum = a;
  for( int i = 0; i < count; ++i )
  {
    // indexing too
    int value = values[ i ];

    // operators are surrounded by spaces
    sum += value;
  }
  return sum;
}

/////////////////
// Line Length //
/////////////////
// No more than 80 characters
1         2         3         4         5         6        7         8
1234567890123456789012345678901234567890123456789012345678912345678901234567890

///////////////
// Functions //
///////////////
// if the argument list is long, pad it into multiple lines with
// each argument on its own line.
//
// don't move them into a struct just for the sake of line length
void Foo(
  int argument0,
  int argument1,
  int argument2,
  int argument3,
  int argument4 );

/////////////////
// Annotations //
/////////////////

// TODO( N8 ): Short description of the task
// NOTE( Bob ): clamping variable foo prevents overflow
// TEMP( Joe ) <-- this better not be pushed to the main branch

//////////////
// Pointers //
//////////////
void* foo;  // ok
void *foo;  // ok
void * foo; // not ok
void*foo;   // not ok

//////////
// Misc //
//////////

// Don't curse in comments
goto // <-- just no
// Follow whatever style is used previously in a file

