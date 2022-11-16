#pragma once

template<typename ...Params>
class Delegate
{
public:
	Delegate& operator+=(std::function<void(Params... params)> func)
	{
		_functions.push_back(func);
		return *this;
	}

	template<typename ...Args>
	void operator()(Args... args)
	{
		for (auto& f : _functions)
		{
			f(args...);
		}
	}
private:
	std::vector<std::function<void(Params... params)>> _functions;
};