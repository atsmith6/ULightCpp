# ULightCpp

ULightCpp is an extremely light weight unit test framework for C++ that make writing unit tests, performance benchmarks, and stress tests from large numbers of concurrent threads trivially simple to implement.

ULight runs as a standalone command line executable into which the tests are built.  The intent is that the executable can be deployed anywhere and run with no external dependencies.

## How-To

To use the test harness create a command line program and add the following files:

- ULightCpp.h
- ULightCpp.cpp
- ULightCppThreadStarter.h
- ULightCppThreadStarter.cpp
- ULightTestTimer.h
- ULightTestTimer.cpp

Now replace the contents of the *main.cpp* file with:

```
#include "ULightCpp.h"

IMPLEMENT_UNITTESTS(std::wcout)
```

To create tests add a new test file (e.g. mytests.cpp) with the
following contents:

```
#include "ULightCpp.h"

UNITTESTFILE

TEST(test_something)
{
	T(false, "Not yet implemented!");
}
```

You can add additional tests by adding more **TEST** functions.  The only restriction is that each test name inside a single command line executable must be unique.  If two tests in different compilation units share the same name then one of them will simply be ignored and which one this will be is compiler specific.

If you run the executable you will get output similar to the following:

```
Test Failed: test_something
 Location: mytests.cpp (7)
 Error: Not yet implemented!

Results (2015-02-16 12:32:52): 
 Passed       0
 Failed       1
 Skipped      0
 Incomplete   0
 Total        1
 Elapsed      431us
 Benchmarking Disabled
```

## Test Code

ULightCpp includes only one test assertion:

```
	T(assertion, "failure message");
```

The failure message expands to a standard library stream object so you can use familiar operators for added information:

```
	T(assertion, "This failed because " << some_error_string << " code " << some_code);
```

## Long Running Tests

Some tests are long running and shouldn't run every time you run the code.  ULightCpp calls these tests stress tests.  Create a stress test as follows:

```
STRESSTEST(test_something)
{
	// Some long running test
}
```

This test will be skipped unless the test executable is run with the `-s` or `--stress` command line argument.

## Linking ULightCpp as a Library

The ULightCpp files can be linked in as a static library instead of directly adding them to the main executable.  The code has however not been written to be housed in a dynamic library or shared object.

## Marking Tests as Incomplete or Skipped

To skip a test add the keyword `SKIPTEST` as the first line of the test.

To mark a test as incomplete add the `INCOMPLETE` keyword as the first line of the test.

## Benchmarking Tests

To benchmark tests simple add the keyword `BENCHMARK` to the top of the test function:

```
TEST(mybenchmarkedThing)
{
	BENCHMARK

	// Write some code here
}
```

When the test runs it will be timed.  To see the benchmark in the output run the tests with the `-b` or `--benchmark`
command line argument.

## Setup and Teardown

If you need to setup an environment for a test before execution use the following function blocks:

```
SETUP(mytest)
{
	// Setup any pre-requisites here
}

TEST(mytest)
{
	// Do your testing here
}

TEARDOWN(mytest)
{
	// Post test teardown
}
```

## Multiple Threads

Sometimes you need to get multiple threads running concurrently to test parallel behaviour.  For example you may set up a TCP/IP server, then hit it with 10 or more client threads to ensure you don't get any race conditions or deadlocks.

To create a multi-threaded test initialise any pre-requisites (like starting the TCP server) in the `SETUP` and `TEARDOWN` functions as detailed already, then create the test task as follows:

```
TEST_TASK(mytest, taskA, 10)
{

}

TEST_TASK(mytest, taskB, 5)
{

}
```

The above example will create 10 tasks of type 'taskA' and 5 tasks of type 'taskB' to be run concurrently.  The test will run until all tasks exit.  Note that the first parameter must match the test name used in the `SETUP` and `TEARDOWN` functions.
