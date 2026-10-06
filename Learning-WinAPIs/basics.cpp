// C++ fundamentals reference sheet
// Build with a modern compiler, for example: c++ -std=c++17 basics.cpp

#include <iostream>
#include <string>
#include <vector>

// Functions can accept parameters and return a value.
int add(int left, int right)
{
	return left + right;
}

// Pass larger objects by const reference to avoid copying them.
void greet(const std::string& name)
{
	std::cout << "Hello, " << name << "!\n";
}

int main()
{
	// Variables and common types
	int age = 20;
	double height = 1.75;
	char grade = 'A';
	bool isLearning = true;
	std::string name = "Alex";
	const int daysInWeek = 7; // const values cannot be reassigned

	std::cout << name << " is " << age << " years old.\n";
	std::cout << "Height: " << height << ", grade: " << grade << '\n';
	std::cout << "Learning: " << isLearning << ", day   s per week: "
			  << daysInWeek << "\n\n";

	// Arithmetic: + - * / %, comparisons: == != < > <= >=
	// Logical operators: && (and), || (or), ! (not)
	int first = 10;
	int second = 3;
	std::cout << "Sum: " << first + second
			  << ", remainder: " << first % second << '\n';
	// Integer division truncates: 10 / 3 is 3.

	// if / else if / else
	if (age < 13)
	{
		std::cout << "You are a child.\n";
	}
	else if (age < 20)
	{
		std::cout << "You are a teenager.\n";
	}
	else
	{
		std::cout << "You are an adult.\n";
	}

	// switch selects a branch based on one value.
	switch (grade)
	{
	case 'A':
		std::cout << "Great grade!\n";
		break;
	case 'B':
		std::cout << "Good grade.\n";
		break;
	default:
		std::cout << "Keep practicing.\n";
		break;
	}

	// for loop: use when you know how many times to repeat.
	for (int i = 0; i < 3; ++i)
	{
		std::cout << "for loop: " << i << '\n';
	}

	// while loop: repeats as long as the condition remains true.
	int count = 0;
	while (count < 3)
	{
		std::cout << "while loop: " << count << '\n';
		++count;
	}

	// do-while always runs at least once.
	int attempts = 0;
	do
	{
		++attempts;
	} while (attempts < 1);

	// break exits a loop; continue skips to the next iteration.
	for (int i = 0; i < 5; ++i)
	{
		if (i == 1)
			continue;
		if (i == 4)
			break;
		std::cout << "loop value: " << i << '\n';
	}

	// Calling functions
	greet(name);
	std::cout << "2 + 3 = " << add(2, 3) << '\n';

	// Fixed-size array; indexes begin at zero.
	int scores[3] = {90, 85, 100};
	std::cout << "First score: " << scores[0] << '\n';

	// vector is a resizable sequence.
	std::vector<int> numbers = {1, 2, 3};
	numbers.push_back(4);
	for (int number : numbers) // range-based for loop
	{
		std::cout << number << ' ';
	}
	std::cout << '\n';

	// A reference is an alias for an existing variable.
	int original = 5;
	int& reference = original;
	reference = 6;
	std::cout << "Original after reference change: " << original << '\n';

	// A pointer stores an address. & gets an address; * dereferences it.
	int value = 42;
	int* pointer = &value;
	std::cout << "Value through pointer: " << *pointer << '\n';
	pointer = nullptr; // nullptr represents a pointer to no object

	// Basic input example (uncomment to try):
	// std::string userName;
	// std::cout << "Enter your name: ";
	// std::cin >> userName;
	// greet(userName);

	return 0; // 0 indicates successful completion
}
