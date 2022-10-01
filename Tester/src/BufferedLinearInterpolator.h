#pragma once

#include <vector>

template<typename T>
class BufferedLinearInterpolator
{
	class BufferedItem
	{
	public:
		BufferedItem()
			: Item(0), Time(0)
		{

		}
		BufferedItem(T item, double timeSent)
			: Item(item), Time(timeSent)
		{

		}
	public:
		T Item{};
		double Time = 0;
	};

public:
	BufferedLinearInterpolator(uint32_t bufferCountLimit)
	{
		_bufferCountLimit = bufferCountLimit;
		_buffer.reserve(_bufferCountLimit);
	}

	void Clear()
	{
		_buffer.clear();
		_endTimeConsumed = 0;
		_beginTimeConsumed = 0;
	}

	void ResetTo(T targetValue, double serverTime)
	{
		_lifetimeConsumedCount = 1;
		_interpBeginValue = targetValue;
		_interpEndValue = targetValue;
		_currentInterpValue = targetValue;
		_buffer.clear();
		_endTimeConsumed = 0;
		_beginTimeConsumed = 0;

		Update(0, serverTime, serverTime);
	}

	void TryConsumeFromBuffer(double renderTime, double serverTime)
	{
		int consumedCount = 0;
		if (renderTime >= _endTimeConsumed)
		{
			BufferedItem itemToInterpolateTo{};
			bool hasValue = false;
			for (int i = _buffer.size() - 1; i >= 0; --i)
			{
				BufferedItem bufferedValue = _buffer[i];
				if (bufferedValue.Time <= serverTime)
				{
					if (!hasValue || bufferedValue.Time > itemToInterpolateTo.Time)
					{
						if (_lifetimeConsumedCount == 0)
						{
							_beginTimeConsumed = bufferedValue.Time;
							_interpBeginValue = bufferedValue.Item;
						}
						else if (consumedCount == 0)
						{
							_beginTimeConsumed = _endTimeConsumed;
							_interpBeginValue = _interpEndValue;
						}

						if (bufferedValue.Time > _endTimeConsumed)
						{
							itemToInterpolateTo = bufferedValue;
							hasValue = true;
							_endTimeConsumed = bufferedValue.Time;
							_interpEndValue = bufferedValue.Item;
						}
					}

					_buffer.erase(_buffer.begin() + i);
					++consumedCount;
					++_lifetimeConsumedCount;
				}
			}
		}
	}

	void AddMeasurement(T newMeasurement, double sentTime)
	{
		++_itemsReceivedThisFrame;

		if (_itemsReceivedThisFrame > _bufferCountLimit)
		{
			if (_lastBufferedItemReceived.Time < sentTime)
			{
				_lastBufferedItemReceived.Item = newMeasurement;
				_lastBufferedItemReceived.Time = sentTime;
				ResetTo(newMeasurement, sentTime);
				_buffer.push_back(_lastBufferedItemReceived);
			}

			return;
		}

		if (sentTime > _endTimeConsumed || _lifetimeConsumedCount == 0)
		{
			_lastBufferedItemReceived.Item = newMeasurement;
			_lastBufferedItemReceived.Time = sentTime;
			_buffer.push_back(_lastBufferedItemReceived);
		}
	}


	float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
	{
		// Based on Game Programming Gems 4 Chapter 1.10
		smoothTime = std::max(0.0001F, smoothTime);
		float omega = 2.0 / smoothTime;

		float x = omega * deltaTime;
		float exp = 1.0 / (1.0 + x + 0.48f * x * x + 0.235f * x * x * x);
		float change = current - target;
		float originalTo = target;

		// Clamp maximum speed
		float maxChange = maxSpeed * smoothTime;
		change = std::clamp(change, -maxChange, maxChange);
		target = current - change;

		float temp = (currentVelocity + omega * change) * deltaTime;
		currentVelocity = (currentVelocity - omega * temp) * exp;
		float output = target + (change + temp) * exp;

		// Prevent overshooting
		if (originalTo - current > 0.0F == output > originalTo)
		{
			output = originalTo;
			currentVelocity = (output - originalTo) / deltaTime;
		}

		return output;
	}

	float currentVelocity = 0;
	T Update(double deltaTime, double renderTime, double serverTime)
	{
		TryConsumeFromBuffer(renderTime, serverTime);

		if (_lifetimeConsumedCount >= 1)
		{
			double t = 1;
			double range = _endTimeConsumed - _beginTimeConsumed;
			if (range > 0.000001)
			{
				t = (renderTime - _beginTimeConsumed) / range;
				if (t < 0)
				{
					std::cout << "Shoud never happen\n";
					t = 0;
				}

				if (t > _maxInterpolationBound)
				{
					t = 1;
				}
			}

			//_currentInterpValue = SmoothDamp(_currentInterpValue, _interpEndValue, currentVelocity, deltaTime / _maximumInterpolationTime, 1000000, deltaTime);
			auto target = InterpolateUnclamped(_interpBeginValue, _interpEndValue, t);
			_currentInterpValue = Interpolate(_currentInterpValue, target, deltaTime / _maximumInterpolationTime);
		}

		_itemsReceivedThisFrame = 0;
		return _currentInterpValue;
	}

	T GetInterpolatedValue()
	{
		return _currentInterpValue;
	}
protected:
	virtual T Interpolate(T start, T end, double time) = 0;
	virtual T InterpolateUnclamped(T start, T end, double time) = 0;
private:
	double _maxInterpolationBound = 3;
	double _maximumInterpolationTime = 0.1;
	T _interpBeginValue{};
	T _currentInterpValue{};
	T _interpEndValue{};

	double _endTimeConsumed = 0;
	double _beginTimeConsumed = 0;

	std::vector<BufferedItem> _buffer;

	uint32_t _bufferCountLimit = 0;
	BufferedItem _lastBufferedItemReceived;
	uint32_t _itemsReceivedThisFrame = 0;
	uint32_t _lifetimeConsumedCount = 0;
};