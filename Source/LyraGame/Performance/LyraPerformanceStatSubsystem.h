// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ChartCreation.h"
#include "LyraPerformanceStatTypes.h"
#include "Algo/MaxElement.h"
#include "Algo/MinElement.h"
#include "Stats/StatsData.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LyraPerformanceStatSubsystem.generated.h"

enum class ELyraDisplayablePerformanceStat : uint8;

class FSubsystemCollectionBase;
class ULyraPerformanceStatSubsystem;
class UObject;
struct FFrame;

/**
 * Stores a buffer of the given sample size and provides an interface to get data
 * like the min, max, and average of that group.
 */
class FSampledStatCache
{
public:

	FSampledStatCache(const int32 InSampleSize = 125)
		: SampleSize(InSampleSize)
	{
		check(InSampleSize > 0);
		
		Samples.Empty();
		Samples.AddZeroed(SampleSize);
	}
		
	void RecordSample(const double Sample)
	{
		// A simple little ring buffer for storing the samples over time
		Samples[CurrentSampleIndex] = Sample;
	
		CurrentSampleIndex++;
		if (CurrentSampleIndex >= Samples.Num())
		{
			CurrentSampleIndex = 0u;
		}
	}

	double GetCurrentCachedStat() const
	{
		return Samples[CurrentSampleIndex];
	}
		
	double GetLastCachedStat() const
	{
		int32 LastIndex = CurrentSampleIndex - 1;
		if (LastIndex < 0)
		{
			LastIndex = Samples.Num() - 1;
		}
			
		return Samples[LastIndex];
	}

	/**
	 * Iterates all the samples, starting at the most recently sampled index
	 */
	void ForEachCurrentSample(const TFunctionRef<void(const double Stat)> Func) const
	{
		int32 Index = CurrentSampleIndex;
	
		for (int32 i = 0; i < SampleSize; i++)
		{
			Func(Samples[Index]);
				
			Index++;
			if (Index == SampleSize)
			{
				Index = 0;
			}
		}
	}

	inline int32 GetSampleSize() const
	{
		return SampleSize;
	}

	inline double GetAverage() const
	{
		double Sum = 0.0;
		ForEachCurrentSample([&Sum](const double Stat)
		{
			Sum += Stat;
		});

		// Return the average stat value
		return Sum / static_cast<double>(SampleSize);
	}

	inline double GetMin() const
	{
		return *Algo::MinElement(Samples);
	}

	inline double GetMax() const
	{
		return *Algo::MaxElement(Samples);
	}
		
private:
	const int32 SampleSize = 125;

	int32 CurrentSampleIndex = 0;
	
	TArray<double> Samples;
};

//////////////////////////////////////////////////////////////////////

// Observer which caches the stats for the previous frame
struct FLyraPerformanceStatCache : public IPerformanceDataConsumer
{
public:
	FLyraPerformanceStatCache(ULyraPerformanceStatSubsystem* InSubsystem)
		: MySubsystem(InSubsystem)
	{
	}

	//~IPerformanceDataConsumer interface
	virtual void StartCharting() override;
	virtual void ProcessFrame(const FFrameData& FrameData) override;
	virtual void StopCharting() override;
	//~End of IPerformanceDataConsumer interface

	/**
	* Returns the latest cached value for the given stat type 
	*/
	double GetCachedStat(ELyraDisplayablePerformanceStat Stat) const;

	/**
	 * Returns a pointer to the cache for the given stat type. This can be used it
	 * get the min/max/average of this stat, the latest stat, and iterate all of the samples.
	 * This is useful for generating some UI, like an FPS chart over time. 
	 */
	const FSampledStatCache* GetCachedStatData(const ELyraDisplayablePerformanceStat Stat) const;

protected:

	void RecordStat(const ELyraDisplayablePerformanceStat Stat, const double Value);
	
	ULyraPerformanceStatSubsystem* MySubsystem;

	/**
	 * Caches the sampled data for each of the performance stats currently available
	 */
	TMap<ELyraDisplayablePerformanceStat, FSampledStatCache> PerfStateCache;
};

//////////////////////////////////////////////////////////////////////

// Subsystem to allow access to performance stats for display purposes
UCLASS(BlueprintType)
class ULyraPerformanceStatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	double GetCachedStat(ELyraDisplayablePerformanceStat Stat) const;

	const FSampledStatCache* GetCachedStatData(const ELyraDisplayablePerformanceStat Stat) const;

	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of USubsystem interface

protected:
	
	TSharedPtr<FLyraPerformanceStatCache> Tracker;
};
