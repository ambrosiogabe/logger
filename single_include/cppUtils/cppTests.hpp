#ifndef GABE_CPP_UTILS_TESTS_H
#define GABE_CPP_UTILS_TESTS_H

#define ADD_TEST(testSuite, testName) Tests::addTest(testSuite, #testName, testName)

#define ASSERT_TRUE(val) { if (!(val)) return "ASSERT_TRUE("#val")"; }
#define ASSERT_FALSE(val) { if (val) return "ASSERT_FALSE("#val")"; }

#define ASSERT_EQUAL(a, b) { if (a != b) return "ASSERT_EQUAL("#a", "#b")"; }
#define ASSERT_NOT_EQUAL(a, b) { if (a == b) return "ASSERT_NOT_EQUAL("#a", "#b")"; }

#define ASSERT_NULL(val) { if (val != nullptr) return "ASSERT_NULL("#val")"; }
#define ASSERT_NOT_NULL(val) { if (val == nullptr) return "ASSERT_NOT_NULL("#val")"; }

#define DEFINE_TEST(fnName) const char* fnName()
#define END_TEST return nullptr

namespace CppUtils { namespace Tests {

struct TestSuite;
typedef const char* (*TestFn)();

TestSuite& addTestSuite(const char* testSuiteName);

void addTest(TestSuite& testSuite, const char* testName, TestFn fn);

void runTests();

void free();

} } // End namespace CppUtils::Tests

#endif // GABE_CPP_UTILS_TESTS_H

#ifdef GABE_CPP_UTILS_IMPL

#include <stdint.h>
#include <vector>

#undef GABE_CPP_UTILS_IMPL
#include "cppUtils/cppUtils.hpp"
#include "cppUtils/cppPrint.hpp"
#define GABE_CPP_UTILS_IMPL

// See here for more escape code colors https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
#define ANSI_COLOR_RED     "\u001b[38;5;167m"
#define ANSI_COLOR_GREEN   "\u001b[38;5;84m"
#define ANSI_COLOR_YELLOW  "\u001b[38;5;220m"
#define ANSI_COLOR_BLUE    "\u001b[34m"
#define ANSI_COLOR_MAGENTA "\u001b[35m"
#define ANSI_COLOR_CYAN    "\u001b[36m"
#define ANSI_COLOR_RESET   "\u001b[0m"

namespace CppUtils { namespace Tests {

// ----------------- Internal structures -----------------
struct TestPrototype
{
	uint8_t* name;
	size_t nameLength;
	TestFn fn;
};

struct TestSuite
{
	const char* name;
	char** testResults;
	TestPrototype* tests;
	size_t testsLength;
	size_t numTestsPassed;
};

// ----------------- Internal variables -----------------
static std::vector<TestSuite> testSuites = {};

// ----------------- Internal functions -----------------
static void runTestSuite(void* testSuiteRaw, size_t testSuiteSize);
static void printTestSuiteResCallback(void* testSuiteRaw, size_t testSuiteSize);

TestSuite& addTestSuite(const char* testSuiteName)
{
	TestSuite res = {};
	res.name = testSuiteName;
	res.numTestsPassed = 0;
	res.tests = nullptr;
	res.testResults = nullptr;
	res.testsLength = 0;
	testSuites.push_back(res);

	return testSuites[testSuites.size() - 1];
}

void addTest(TestSuite& testSuite, const char* testName, TestFn fn)
{
	testSuite.testsLength++;
	testSuite.tests = (TestPrototype*)g_memory_realloc(
		testSuite.tests,
		sizeof(TestPrototype) * testSuite.testsLength
	);
	testSuite.testResults = (char**)g_memory_realloc(
		testSuite.testResults,
		sizeof(char**) * testSuite.testsLength
	);

	TestPrototype test = {};
	test.fn = fn;
	test.nameLength = std::strlen(testName);
	test.name = (uint8*)g_memory_allocate(sizeof(uint8) * (test.nameLength + 1));
	g_memory_copyMem(test.name, (void*)testName, sizeof(uint8) * test.nameLength);
	test.name[test.nameLength] = '\0';

	testSuite.tests[testSuite.testsLength - 1] = test;
	testSuite.testResults[testSuite.testsLength - 1] = nullptr;
}

void runTests()
{
	g_logger_info("Running {} test {}.\n", testSuites.size(), testSuites.size() > 1 ? "suites" : "suite");

	// TODO: Add thread pool implementation
	//GlobalThreadPool threadPool = GlobalThreadPool(std::thread::hardware_concurrency());

	//for (const auto& testSuite : testSuites)
	//{
	//	threadPool.queueTask(
	//		runTestSuite,
	//		testSuite.name,
	//		(void*)&testSuite,
	//		sizeof(TestSuite),
	//		MathAnim::Priority::None,
	//		printTestSuiteResCallback
	//	);
	//}

	//threadPool.beginWork();

	//// Should join all threads and wait for tasks to finish
	//threadPool.free();

	for (const auto& testSuite : testSuites)
	{
		runTestSuite((void*)&testSuite, sizeof(TestSuite));
		printTestSuiteResCallback((void*)&testSuite, sizeof(TestSuite));
	}

	size_t numTestSuitesPassed = 0;
	for (const auto& testSuite : testSuites)
	{
		if (testSuite.numTestsPassed >= testSuite.testsLength)
		{
			numTestSuitesPassed++;
		}
	}

	IO::printf("\n  Number of Test Suites Passed "
		ANSI_COLOR_YELLOW
		"{}/{}\n\n"
		ANSI_COLOR_RESET,
		numTestSuitesPassed,
		testSuites.size()
	);
}

void free()
{
	for (auto& testSuite : testSuites)
	{
		for (size_t i = 0; i < testSuite.testsLength; i++)
		{
			if (testSuite.tests[i].name)
			{
				g_memory_free(testSuite.tests[i].name);
			}

			if (testSuite.testResults[i])
			{
				g_memory_free(testSuite.testResults[i]);
			}

			testSuite.tests[i].name = nullptr;
			testSuite.tests[i].nameLength = 0;
			testSuite.tests[i].fn = nullptr;
		}

		if (testSuite.tests)
		{
			g_memory_free(testSuite.tests);
		}

		if (testSuite.testResults)
		{
			g_memory_free(testSuite.testResults);
		}

		testSuite.tests = nullptr;
		testSuite.testResults = nullptr;
		testSuite.numTestsPassed = 0;
		testSuite.name = nullptr;
		testSuite.testsLength = 0;
	}

	testSuites.clear();
}

// ----------------- Internal functions -----------------
static void runTestSuite(void* testSuiteRaw, size_t testSuiteSize)
{
	g_logger_assert(testSuiteSize == sizeof(TestSuite), "Invalid data passed to runTestSuite");
	TestSuite* testSuite = (TestSuite*)testSuiteRaw;

	for (size_t i = 0; i < testSuite->testsLength; i++)
	{
		const char* result = testSuite->tests[i].fn();
		if (result == nullptr)
		{
			testSuite->numTestsPassed++;
			testSuite->testResults[i] = nullptr;
		}
		else
		{
			size_t strLength = std::strlen(result) + 1;
			testSuite->testResults[i] = (char*)g_memory_allocate(sizeof(char) * strLength);
			g_memory_copyMem(testSuite->testResults[i], (void*)result, strLength);
		}
	}
}

static void printTestSuiteResCallback(void* testSuiteRaw, size_t testSuiteSize)
{
	g_logger_assert(testSuiteSize == sizeof(TestSuite), "Invalid data passed to printTestSuiteResCallback");
	TestSuite* testSuite = (TestSuite*)testSuiteRaw;

	IO::printf("  Test Suite '{}' Results...\n\n", testSuite->name);

	for (size_t i = 0; i < testSuite->testsLength; i++)
	{
		if (testSuite->testResults[i] == nullptr)
		{
			IO::printf(
				ANSI_COLOR_GREEN
				u8"      \u2713 Success "
				ANSI_COLOR_RESET
				"'{}::{}'\n",
				testSuite->name,
				testSuite->tests[i].name
			);
		}
		else
		{
			IO::printf(
				ANSI_COLOR_RED
				u8"      \u0078 Fail    "
				ANSI_COLOR_RESET
				"'{}::{}'\n"
				"        Failed at: "
				ANSI_COLOR_RED
				"{}\n"
				ANSI_COLOR_RESET,
				testSuite->name,
				testSuite->tests[i].name,
				testSuite->testResults[i]
			);
		}
	}

	if (testSuite->numTestsPassed < testSuite->testsLength)
	{
		IO::printf(
			ANSI_COLOR_RED
			"\n    Suite Fail "
			ANSI_COLOR_RESET
			"'{}'",
			testSuite->name
		);
	}
	else
	{
		IO::printf(
			ANSI_COLOR_GREEN
			"\n    Suite Success "
			ANSI_COLOR_RESET
			"'{}'",
			testSuite->name
		);
	}

	IO::printf("\n    Number of Tests Passed "
		ANSI_COLOR_YELLOW
		"{}/{}\n\n"
		ANSI_COLOR_RESET,
		testSuite->numTestsPassed,
		testSuite->testsLength
	);
}

} } // End namespace CppUtils::Tests

#endif 