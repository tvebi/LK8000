/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2 or later
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "AATDistance.h"
#include "CalcTask.h"
#include "NavFunctions.h"


bool InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;

  ScopeLock lock(CritSec_TaskData);

  if (!ValidFinish(Basic, Calculated)) {
    return false;
  }

  // Finish invalid
  if (!ValidTaskPointFast(i)) {
    return false;
  }

  // distance from aircraft to start point
  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  WayPointList[Task[i].Index].Latitude, 
                  WayPointList[Task[i].Index].Longitude,
                  &FirstPointDistance,
                  &AircraftBearing);

  bool inrange = (FirstPointDistance < FinishRadius);
  if (!inrange) {
    LastInSector = false;
  }

  if (FinishLine == sector_type_t::CIRCLE) {
    return inrange;
  }

  // Finish line
  AircraftBearing = AngleLimit180(AircraftBearing - Task[i].InBound);

  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if (FinishLine == sector_type_t::LINE) {  // Finish line
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  } else {
    // FAI 90 degree
    approaching = !((AircraftBearing >= 135) || (AircraftBearing <= -135));
  }

  if (inrange) {
    if (LastInSector) {
      // previously approaching the finish line
      if (!approaching) {
        // now moving away from finish line
        LastInSector = false;
        return true;
      }
    } else {
      if (approaching) {
        // now approaching the finish line
        LastInSector = true;
      }
    }
  } else {
    LastInSector = false;
  }
  return false;
}

