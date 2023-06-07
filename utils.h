#pragma once
#include <list>
#include <atomic>


struct Node
{
	int val;
	Node* next;
};

std::atomic<Node*> list_head(nullptr);

void append(int val) {
	Node* oldHead = list_head;
	Node* newNode = new Node{ val,oldHead };

	// what follows is equivalent to: list_head = newNode, but in a thread-safe way:
	while (!list_head.compare_exchange_weak(oldHead, newNode))
		newNode->next = oldHead;
}

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}

	std::list<T> result;

	//cut first value to result
	result.splice(result.begin(), input, input.begin());

	T const& pivot = *result.begin();
	//return second iterator, matched in front
	auto divide_point = std::partition(input.begin(), input.end(),[&](T const& t) { return t < pivot; });
	
	std::list<T> lower_part;
	lower_part.splice(lower_part.end(), input, input.begin(),divide_point);

	std::future<std::list<T> > new_lower(std::async(&parallel_quick_sort<T>, std::move(lower_part)));
	
	auto new_higher(parallel_quick_sort(std::move(input)));
	//begin|pivot|end
	result.splice(result.end(), new_higher); 
	result.splice(result.begin(), new_lower.get());

	//begin|lower|pivot|high|end
	return result;
}