//  $Header:
//------------------------------------------------------------------------------------------------
//
//  FILE:    CvDate.h
//
//  PURPOSE: Class to keep a Civ4 date and methods related to the time/turn relationship
//
//------------------------------------------------------------------------------------------------

#include "FProfiler.h"

#include "CvGameCoreDLL.h"
#include "CvGameCoreUtils.h"
#include "CvGameAI.h"
#include "CvGlobals.h"
#include "CvInfos.h"

CvDate::CvDate()
{
	m_iTick = 0;
}

CvDate::CvDate(uint32_t iTick) :
m_iTick(iTick)
{
}

int CvDate::getYear() const
{
	return GC.getGame().getStartYear() + (m_iTick / 360);
}

int CvDate::getDay() const
{
	return ((m_iTick % 360) % 30) + 1;
}

uint32_t CvDate::GetTick() const
{
	return m_iTick;
}

void CvDate::setTick(uint32_t newTick) 
{
	m_iTick = newTick;
}

int CvDate::getMonth() const
{
	return (m_iTick % 360) / 30;
}

int CvDate::getWeek() const
{
	return ((getDay() - 1) / 7) + 1;
}

SeasonTypes CvDate::getSeason() const
{
	const int month = getMonth();

	if (month <= 1 || month >= 11)
	{
		return SEASON_WINTER; // Winter
	}
	if (month >= 2 && month <= 4)
	{
		return SEASON_SPRING; // Spring
	}
	if (month >= 5 && month <= 7)
	{
		return SEASON_SUMMER; // Summer
	}
	if (month >= 8 && month <= 10)
	{
		return SEASON_AUTUMN; // Autumn
	}
	return NO_SEASON; // This will never be executed
}


CvDateIncrement CvDate::getIncrement(GameSpeedTypes eGameSpeed) const
{
	PROFILE_EXTRA_FUNC();
	GameSpeedTypes eActualGameSpeed = eGameSpeed;
	if (eGameSpeed == NO_GAMESPEED)
	{
		eActualGameSpeed = GC.getGame().getGameSpeedType();
	}
	CvGameSpeedInfo& kInfo = GC.getGameSpeedInfo(eActualGameSpeed);
	const std::vector<CvDateIncrement>& aIncrements = kInfo.getIncrements();
	if (!kInfo.getEndDatesCalculated())
	{
		calculateEndDates(eActualGameSpeed);
	}

	foreach_(const CvDateIncrement& it, aIncrements)
	{
		if (*this < it.m_endDate)
		{
			return it;
		}
	}
	return aIncrements[aIncrements.size()-1];
}

void CvDate::increment(GameSpeedTypes eGameSpeed)
{
	const CvDateIncrement inc = getIncrement(eGameSpeed);

	double dateModifier = 1.0;
	GameSpeedTypes eActualGameSpeed = eGameSpeed;
	if (eGameSpeed == NO_GAMESPEED)
	{
		eActualGameSpeed = GC.getGame().getGameSpeedType();
	}
	CvGameSpeedInfo& kInfo = GC.getGameSpeedInfo(eActualGameSpeed);

	int incrementValue = 0;
	incrementValue += static_cast<uint32_t>(inc.m_iIncrementDay);
	incrementValue += static_cast<uint32_t>(inc.m_iIncrementMonth * 30);


	m_iTick += incrementValue;
}

void CvDate::increment(int iTurns, GameSpeedTypes eGameSpeed)
{
	PROFILE_EXTRA_FUNC();
	for (int i = 0; i < iTurns; i++)
	{
		increment(eGameSpeed);
	}
}

bool CvDate::operator !=(const CvDate &kDate) const
{
	return !(*this == kDate);
}

bool CvDate::operator ==(const CvDate &kDate) const
{
	return m_iTick == kDate.GetTick();
}

bool CvDate::operator <(const CvDate &kDate) const
{
	return m_iTick < kDate.GetTick();
}

bool CvDate::operator >(const CvDate &kDate) const
{
	return m_iTick > kDate.GetTick();
}

bool CvDate::operator <=(const CvDate &kDate) const
{
	return ! (*this > kDate);
}

bool CvDate::operator >=(const CvDate &kDate) const
{
	return ! (*this < kDate);
}

CvDate CvDate::getStartingDate()
{
	return CvDate();
}

CvDate CvDate::getDate(int iTurn, GameSpeedTypes eGameSpeed)
{
	PROFILE_EXTRA_FUNC();
	CvDate date;
	int iRemainingTurns = 0;

	GameSpeedTypes eActualGameSpeed = eGameSpeed;
	if (eGameSpeed == NO_GAMESPEED)
	{
		eActualGameSpeed = GC.getGame().getGameSpeedType();
	}
	CvGameSpeedInfo& kInfo = GC.getGameSpeedInfo(eActualGameSpeed);
	const std::vector<CvDateIncrement>& aIncrements = kInfo.getIncrements();
	if (!kInfo.getEndDatesCalculated())
	{
		calculateEndDates(eActualGameSpeed);
	}


	for (int i=0; i<(int)aIncrements.size(); i++)
	{
		if (iTurn <= aIncrements[i].m_iendTurn)
		{
			if (i==0)
			{
				iRemainingTurns = iTurn;
				date = getStartingDate();
			}
			else
			{
				iRemainingTurns = iTurn - aIncrements[i-1].m_iendTurn;
				date = aIncrements[i-1].m_endDate;
			}
			break;
		}
		else
		{
			iRemainingTurns = iTurn - aIncrements[i].m_iendTurn;
			date = aIncrements[i].m_endDate;
		}
	}

	//date.increment(iRemainingTurns, eGameSpeed);
	bool bHistoricalCalendar = GC.getGame().isModderGameOption(MODDERGAMEOPTION_USE_HISTORICAL_ACCURATE_CALENDAR);
	if (bHistoricalCalendar) 
	{
		uint32_t currentTick = calculateCurrentTick();

		if (currentTick > date.GetTick())
		{
			date.setTick(currentTick);
		}
	}
	else
	{
		date.increment(iRemainingTurns, eGameSpeed);
	}
	return date;
}

void CvDate::calculateEndDates(GameSpeedTypes eGameSpeed)
{
	PROFILE_EXTRA_FUNC();
	CvGameSpeedInfo& kInfo = GC.getGameSpeedInfo(eGameSpeed);
	std::vector<CvDateIncrement>& aIncrements = kInfo.getIncrements();
	kInfo.setEndDatesCalculated(true);
	for (int i=0; i<(int)aIncrements.size(); i++)
	{
		aIncrements[i].m_endDate = CvDate(MAX_UNSIGNED_INT);
		aIncrements[i].m_endDate = getDate(aIncrements[i].m_iendTurn, eGameSpeed);
	}
}