// $Id$

#include "config.h"

#include <qdatetime.h>
#include <qstring.h>
#include <qlist.h>
#include <qregexp.h>
#include <qclipboard.h>
#include <qdialog.h>
#include <qfile.h>

#include <kapp.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <klocale.h>

extern "C" {
  #include <ical.h>
  #include <icalss.h>
  #include <icalparser.h>
  #include <icalrestriction.h>
}

#include "qdatelist.h"
#include "calendar.h"
#include "journal.h"
#include "icalformat.h"

#include "icalformatimpl.h"

#define _ICAL_VERSION "2.0"

using namespace KCal;

ICalFormatImpl::ICalFormatImpl(ICalFormat *parent, Calendar *cal)
{
  mParent = parent;
  mCalendar = cal;
}

ICalFormatImpl::~ICalFormatImpl()
{
}

icalcomponent *ICalFormatImpl::writeTodo(Todo *todo)
{
  QString tmpStr;
  QStringList tmpStrList;

  icalcomponent *vtodo = icalcomponent_new(ICAL_VTODO_COMPONENT);

  writeIncidence(vtodo,todo);

  // due date
  if (todo->hasDueDate()) {
    icaltimetype due;
    if (todo->doesFloat()) {
      due = writeICalDate(todo->dtDue().date());
    } else {
      due = writeICalDateTime(todo->dtDue());
    }
    icalcomponent_add_property(vtodo,icalproperty_new_due(due));
  }

  // completion date
  if (todo->isCompleted()) {
    if (!todo->hasCompletedDate()) {
      // If todo was created by KOrganizer <2.2 it has no correct completion
      // date. Set it to now.
      todo->setCompleted(QDateTime::currentDateTime());
    }
    icaltimetype completed = writeICalDateTime(todo->completed());
    icalcomponent_add_property(vtodo,icalproperty_new_completed(completed));
  }
  
  icalcomponent_add_property(vtodo,
      icalproperty_new_percentcomplete(todo->percentComplete()));
      
  return vtodo;
}

icalcomponent *ICalFormatImpl::writeEvent(Event *event)
{
  kdDebug() << "Write Event '" << event->summary() << "' (" << event->VUID()
              << ")" << endl;
 
  QString tmpStr;
  QStringList tmpStrList;

  icalcomponent *vevent = icalcomponent_new(ICAL_VEVENT_COMPONENT);

  writeIncidence(vevent,event);

  // end time
  icaltimetype end;
  if (event->doesFloat()) {
    kdDebug() << "�� Event " << event->summary() << " floats." << endl; 
    end = writeICalDate(event->dtEnd().date());
  } else {
    kdDebug() << "�� Event " << event->summary() << " has time." << endl; 
    end = writeICalDateTime(event->dtEnd());
  }
  icalcomponent_add_property(vevent,icalproperty_new_dtend(end));

// TODO: find correspondence to secrecy in iCal, DONE: it's CLASS
// TODO: implement secrecy
  // secrecy
//  addPropValue(vevent, VCClassProp, anEvent->getSecrecyStr().ascii());


// TODO: attachements, resources
#if 0
  // attachments
  tmpStrList = anEvent->attachments();
  for ( QStringList::Iterator it = tmpStrList.begin();
        it != tmpStrList.end();
        ++it )
    addPropValue(vevent, VCAttachProp, (*it).ascii());
  
  // resources
  tmpStrList = anEvent->resources();
  tmpStr = tmpStrList.join(";");
  if (!tmpStr.isEmpty())
    addPropValue(vevent, VCResourcesProp, tmpStr.ascii());

#endif

// TODO: transparency
  // transparency
//  tmpStr.sprintf("%i",anEvent->getTransparency());
//  addPropValue(vevent, VCTranspProp, tmpStr.ascii());
  
  return vevent;
}

icalcomponent *ICalFormatImpl::writeJournal(Journal *journal)
{
  icalcomponent *vjournal = icalcomponent_new(ICAL_VJOURNAL_COMPONENT);

  writeIncidence(vjournal,journal);
  
  return vjournal;
}

void ICalFormatImpl::writeIncidence(icalcomponent *parent,Incidence *incidence)
{
  // creation date
  icalcomponent_add_property(parent,icalproperty_new_created(
      writeICalDateTime(incidence->created())));

  // unique id
  icalcomponent_add_property(parent,icalproperty_new_uid(
      incidence->VUID().latin1()));

  // revision
  icalcomponent_add_property(parent,icalproperty_new_sequence(
      incidence->revision()));

  // last modification date
  icalcomponent_add_property(parent,icalproperty_new_lastmodified(
      writeICalDateTime(incidence->lastModified())));

  icalcomponent_add_property(parent,icalproperty_new_dtstamp(
      writeICalDateTime(QDateTime::currentDateTime())));

  // organizer stuff
  icalcomponent_add_property(parent,icalproperty_new_organizer(
      ("MAILTO:" + incidence->organizer()).utf8()));

  // attendees
  if (incidence->attendeeCount() != 0) {
    QList<Attendee> al = incidence->attendees();
    QListIterator<Attendee> ai(al);
    Attendee *curAttendee;
    
    QString attendee;
    for (; ai.current(); ++ai) {
      curAttendee = ai.current();
      if (!curAttendee->email().isEmpty() &&
          !curAttendee->name().isEmpty()) {
        attendee = curAttendee->name() + " <" + curAttendee->email() +
                   ">";
      } else if (curAttendee->name().isEmpty()) {
        attendee = curAttendee->email();
      } else if (curAttendee->email().isEmpty()) {
        attendee = curAttendee->name();
      } else if (curAttendee->name().isEmpty() && 
	         curAttendee->email().isEmpty()) {
        attendee = "";
	kdDebug() << "warning! this Event has an attendee w/o name or email!"
                  << endl;
      } else {
        attendee = "";
      }
      
      icalproperty *p = icalproperty_new_attendee(
          ("MAILTO:" + attendee).utf8());
      icalproperty_add_parameter(p,icalparameter_new_rsvp(
          curAttendee->RSVP() ? ICAL_RSVP_TRUE : ICAL_RSVP_FALSE ));
// TODO: attendee status
//      addPropValue(aProp, VCStatusProp, curAttendee->getStatusStr().ascii());

      icalcomponent_add_property(parent,p);
    }
  }

  // start time
  icaltimetype start;
  if (incidence->doesFloat()) {
    kdDebug() << "�� Incidence " << incidence->summary() << " floats." << endl; 
    start = writeICalDate(incidence->dtStart().date());
  } else {
    kdDebug() << "�� incidence " << incidence->summary() << " has time." << endl; 
    start = writeICalDateTime(incidence->dtStart());
  }
  icalcomponent_add_property(parent,icalproperty_new_dtstart(start));

  // description
  if (!incidence->description().isEmpty()) {
    icalcomponent_add_property(parent,icalproperty_new_description(
        incidence->description().utf8()));
  }

  // summary
  if (!incidence->summary().isEmpty()) {
    icalcomponent_add_property(parent,icalproperty_new_summary(
        incidence->summary().utf8()));
  }

// TODO:
  // status
//  addPropValue(parent, VCStatusProp, incidence->getStatusStr().ascii());

  // priority
  icalcomponent_add_property(parent,icalproperty_new_priority(
      incidence->priority()));

  // categories
  QStringList categories = incidence->categories();
  QStringList::Iterator it;
  for(it = categories.begin(); it != categories.end(); ++it ) {
    icalcomponent_add_property(parent,icalproperty_new_categories((*it).utf8()));
  }
// TODO: Ensure correct concatenation of categories properties.

/*
  // categories
  tmpStrList = incidence->getCategories();
  tmpStr = "";
  QString catStr;
  for ( QStringList::Iterator it = tmpStrList.begin();
        it != tmpStrList.end();
        ++it ) {
    catStr = *it;
    if (catStr[0] == ' ')
      tmpStr += catStr.mid(1);
    else
      tmpStr += catStr;
    // this must be a ';' character as the vCalendar specification requires!
    // vcc.y has been hacked to translate the ';' to a ',' when the vcal is
    // read in.
    tmpStr += ";";
  }
  if (!tmpStr.isEmpty()) {
    tmpStr.truncate(tmpStr.length()-1);
    icalcomponent_add_property(parent,icalproperty_new_categories(
        writeText(incidence->getCategories().join(";"))));
  }
*/

  // related event
  if (incidence->relatedTo()) {
    icalcomponent_add_property(parent,icalproperty_new_relatedto(
        incidence->relatedTo()->VUID().latin1()));
  }

  // pilot sync stuff
  icalproperty *p =
      icalproperty_new_x((QString::number(incidence->pilotId())).utf8());
  icalproperty_set_x_name(p,"X-PILOTID");
  icalcomponent_add_property(parent,p);  
  p = icalproperty_new_x((QString::number(incidence->syncStatus())).utf8());
  icalproperty_set_x_name(p,"X-PILOTSTAT");
  icalcomponent_add_property(parent,p);
  
  // recurrence rule stuff
  KORecurrence *recur = incidence->recurrence();
  if (recur->doesRecur()) {
    kdDebug() << "Write recurrence for '" << incidence->summary() << "' (" << incidence->VUID()
              << ")" << endl;
    icalcomponent_add_property(parent,writeRecurrenceRule(recur));
  }

// TODO: exdates
  QDateList dateList = incidence->exDates();
  for(QDate *date = dateList.first(); date; date = dateList.next()) {
    icalcomponent_add_property(parent,icalproperty_new_exdate(
        writeICalDate(*date)));
  }

  // alarms
  KOAlarm *alarm = incidence->alarm();
  if (alarm->enabled()) {
    kdDebug() << "Write alarm for " << incidence->summary() << endl;
    icalcomponent_add_component(parent,writeAlarm(alarm));
  }
}

icalproperty *ICalFormatImpl::writeRecurrenceRule(KORecurrence *recur)
{
  kdDebug() << "ICalFormatImpl::writeRecurrenceRule()" << endl;

  icalrecurrencetype r;

  icalrecurrencetype_clear(&r);

  int index = 0;
  int index2 = 0;

  QList<KORecurrence::rMonthPos> tmpPositions;
  QList<int> tmpDays;
  int *tmpDay;
  KORecurrence::rMonthPos *tmpPos;
  int day;

  switch(recur->doesRecur()) {
    case KORecurrence::rDaily:
      r.freq = ICAL_DAILY_RECURRENCE;
#if 0
      tmpStr.sprintf("D%i ",anEvent->rFreq);
//      if (anEvent->rDuration > 0)
//	tmpStr += "#";
#endif
      break;
    case KORecurrence::rWeekly:
      r.freq = ICAL_WEEKLY_RECURRENCE;
      for (int i = 0; i < 7; i++) {
	if (recur->days().testBit(i)) {
          if (i == 6) day = 1;
          else day = i + 2;
          r.by_day[index++] = icalrecurrencetype_day_day_of_week(day);
        }
      }
//      r.by_day[index] = ICAL_RECURRENCE_ARRAY_MAX;
#if 0
      tmpStr.sprintf("W%i ",anEvent->rFreq);
      for (int i = 0; i < 7; i++) {
	if (anEvent->rDays.testBit(i))
	  tmpStr += dayFromNum(i);
      }
#endif
      break;
    case KORecurrence::rMonthlyPos:
      r.freq = ICAL_MONTHLY_RECURRENCE;

      tmpPositions = recur->monthPositions();
      tmpPos = tmpPositions.first();
      r.by_set_pos[index2++] = tmpPos->rPos;
      for (int i = 0; i < 7; i++) {
	if (tmpPos->rDays.testBit(i)) {
          if (i == 6) day = 1;
          else day = i + 2;
          r.by_day[index++] = icalrecurrencetype_day_day_of_week(day);
        }
      }
//      r.by_month_day[index] = ICAL_RECURRENCE_ARRAY_MAX;
#if 0
      tmpStr.sprintf("MP%i ", anEvent->rFreq);
      // write out all rMonthPos's
      tmpPositions = anEvent->rMonthPositions;
      for (tmpPos = tmpPositions.first();
	   tmpPos;
	   tmpPos = tmpPositions.next()) {
	
	tmpStr2.sprintf("%i", tmpPos->rPos);
	if (tmpPos->negative)
	  tmpStr2 += "- ";
	else
	  tmpStr2 += "+ ";
	tmpStr += tmpStr2;
	for (int i = 0; i < 7; i++) {
	  if (tmpPos->rDays.testBit(i))
	    tmpStr += dayFromNum(i);
	}
      } // loop for all rMonthPos's
#endif
      break;
    case KORecurrence::rMonthlyDay:
      r.freq = ICAL_MONTHLY_RECURRENCE;

      tmpDays = recur->monthDays();
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
        r.by_month_day[index++] = icalrecurrencetype_day_position(*tmpDay);
      }
//      r.by_day[index] = ICAL_RECURRENCE_ARRAY_MAX;
#if 0    
      tmpStr.sprintf("MD%i ", anEvent->rFreq);
      // write out all rMonthDays;
      tmpDays = anEvent->rMonthDays;
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
	tmpStr2.sprintf("%i ", *tmpDay);
	tmpStr += tmpStr2;
      }
#endif
      break;
    case KORecurrence::rYearlyMonth:
      r.freq = ICAL_YEARLY_RECURRENCE;

      tmpDays = recur->yearNums();
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
        r.by_month[index++] = *tmpDay;
      }
//      r.by_set_pos[index] = ICAL_RECURRENCE_ARRAY_MAX;
#if 0
      tmpStr.sprintf("YM%i ", anEvent->rFreq);
      // write out all the rYearNums;
      tmpDays = anEvent->rYearNums;
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
	tmpStr2.sprintf("%i ", *tmpDay);
	tmpStr += tmpStr2;
      }
#endif
      break;
    case KORecurrence::rYearlyDay:
      r.freq = ICAL_YEARLY_RECURRENCE;

      tmpDays = recur->yearNums();
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
        r.by_year_day[index++] = *tmpDay;
      }
//      r.by_year_day[index] = ICAL_RECURRENCE_ARRAY_MAX;
#if 0
      tmpStr.sprintf("YD%i ", anEvent->rFreq);
      // write out all the rYearNums;
      tmpDays = anEvent->rYearNums;
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
	tmpStr2.sprintf("%i ", *tmpDay);
	tmpStr += tmpStr2;
      }
#endif
      break;
    default:
      r.freq = ICAL_NO_RECURRENCE;
      kdDebug() << "ICalFormatImpl::writeRecurrence(): no recurrence" << endl;
      break;
  }

  r.interval = recur->frequency();

  if (recur->duration() > 0) {
    r.count = recur->duration();
  } else if (recur->duration() == -1) {
    r.count = 0;
  } else {
    r.until = writeICalDate(recur->endDate());
  }

  const char *str = icalrecurrencetype_as_string(&r);
  if (str) {
    kdDebug() << " String: " << str << endl;
  } else {
    kdDebug() << " No String" << endl;
  }

  return icalproperty_new_rrule(r);

#if 0
    // some more variables
    QList<Event::rMonthPos> tmpPositions;
    QList<int> tmpDays;
    int *tmpDay;
    Event::rMonthPos *tmpPos;
    QString tmpStr2;

    switch(anEvent->doesRecur()) {
    case Event::rDaily:
      tmpStr.sprintf("D%i ",anEvent->rFreq);
//      if (anEvent->rDuration > 0)
//	tmpStr += "#";
      break;
    case Event::rWeekly:
      tmpStr.sprintf("W%i ",anEvent->rFreq);
      for (int i = 0; i < 7; i++) {
	if (anEvent->rDays.testBit(i))
	  tmpStr += dayFromNum(i);
      }
      break;
    case Event::rMonthlyPos:
      tmpStr.sprintf("MP%i ", anEvent->rFreq);
      // write out all rMonthPos's
      tmpPositions = anEvent->rMonthPositions;
      for (tmpPos = tmpPositions.first();
	   tmpPos;
	   tmpPos = tmpPositions.next()) {
	
	tmpStr2.sprintf("%i", tmpPos->rPos);
	if (tmpPos->negative)
	  tmpStr2 += "- ";
	else
	  tmpStr2 += "+ ";
	tmpStr += tmpStr2;
	for (int i = 0; i < 7; i++) {
	  if (tmpPos->rDays.testBit(i))
	    tmpStr += dayFromNum(i);
	}
      } // loop for all rMonthPos's
      break;
    case Event::rMonthlyDay:
      tmpStr.sprintf("MD%i ", anEvent->rFreq);
      // write out all rMonthDays;
      tmpDays = anEvent->rMonthDays;
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
	tmpStr2.sprintf("%i ", *tmpDay);
	tmpStr += tmpStr2;
      }
      break;
    case Event::rYearlyMonth:
      tmpStr.sprintf("YM%i ", anEvent->rFreq);
      // write out all the rYearNums;
      tmpDays = anEvent->rYearNums;
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
	tmpStr2.sprintf("%i ", *tmpDay);
	tmpStr += tmpStr2;
      }
      break;
    case Event::rYearlyDay:
      tmpStr.sprintf("YD%i ", anEvent->rFreq);
      // write out all the rYearNums;
      tmpDays = anEvent->rYearNums;
      for (tmpDay = tmpDays.first();
	   tmpDay;
	   tmpDay = tmpDays.next()) {
	tmpStr2.sprintf("%i ", *tmpDay);
	tmpStr += tmpStr2;
      }
      break;
    default:
      kdDebug() << "ERROR, it should never get here in eventToVEvent!" << endl;
      break;
    } // switch

    if (anEvent->rDuration > 0) {
      tmpStr2.sprintf("#%i",anEvent->rDuration);
      tmpStr += tmpStr2;
    } else if (anEvent->rDuration == -1) {
      tmpStr += "#0"; // defined as repeat forever
    } else {
      tmpStr += qDateTimeToISO(anEvent->rEndDate, FALSE);
    }
    addPropValue(vevent,VCRRuleProp, tmpStr.ascii());

  } // event repeats
#endif
}

icalcomponent *ICalFormatImpl::writeAlarm(KOAlarm *alarm)
{
  icalcomponent *a = icalcomponent_new(ICAL_VALARM_COMPONENT);
  
  icalproperty_action action;
  icalattachtype *attach;
  
  if (!alarm->programFile().isEmpty()) {
    action = ICAL_ACTION_PROCEDURE;
// The attachement crashes this function. Find the cause and reenable the code
// later.
#if 0
    attach = icalattachtype_new();
    icalattachtype_set_url(attach,QFile::encodeName(alarm->programFile()).data());
    icalcomponent_add_property(a,icalproperty_new_attach(*attach));
#endif
  } else if (!alarm->audioFile().isEmpty()) {
    action = ICAL_ACTION_AUDIO;
#if 0
    attach = icalattachtype_new();
    icalattachtype_set_url(attach,QFile::encodeName(alarm->audioFile()).data());
    icalcomponent_add_property(a,icalproperty_new_attach(*attach));
#endif
  } else if (!alarm->mailAddress().isEmpty()) {
    action = ICAL_ACTION_EMAIL;
    icalcomponent_add_property(a,icalproperty_new_attendee(alarm->mailAddress()));
    // TODO: multiple attendees can be specified
    icalcomponent_add_property(a,icalproperty_new_summary(alarm->mailSubject()));
    icalcomponent_add_property(a,icalproperty_new_description(alarm->text()));
  } else {
    action = ICAL_ACTION_DISPLAY;
    icalcomponent_add_property(a,icalproperty_new_description(alarm->text()));
  }
  icalcomponent_add_property(a,icalproperty_new_action(action));

  icaltriggertype trigger;
  trigger.time = writeICalDateTime(alarm->time());
  icalcomponent_add_property(a,icalproperty_new_trigger(trigger));

  if (alarm->repeatCount()) {
    icalcomponent_add_property(a,icalproperty_new_repeat(alarm->repeatCount()));
    icalcomponent_add_property(a,icalproperty_new_duration(
                             icaldurationtype_from_int(alarm->snoozeTime()*60)));
  }

  return a;
}

Todo *ICalFormatImpl::readTodo(icalcomponent *vtodo)
{
  Todo *todo = new Todo;

  readIncidence(vtodo,todo);

  icalproperty *p = icalcomponent_get_first_property(vtodo,ICAL_ANY_PROPERTY);

  const char *text;
//  int intvalue;
  icaltimetype icaltime;

  QStringList categories;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_DUE_PROPERTY:  // due date
        icaltime = icalproperty_get_due(p);
        if (icaltime.is_date) {
          todo->setDtDue(QDateTime(readICalDate(icaltime),QTime(0,0,0)));
          todo->setFloats(true);
        } else {
          todo->setDtDue(readICalDateTime(icaltime));
          todo->setFloats(false);
        }
        todo->setHasDueDate(true);
        break;

      case ICAL_COMPLETED_PROPERTY:  // completion date
        icaltime = icalproperty_get_completed(p);
        todo->setCompleted(readICalDateTime(icaltime));
        break;

      case ICAL_PERCENTCOMPLETE_PROPERTY:  // Percent completed
        todo->setPercentComplete(icalproperty_get_percentcomplete(p));
        break;

      case ICAL_RELATEDTO_PROPERTY:  // releated todo (parent)
        text = icalproperty_get_relatedto(p);
        todo->setRelatedToVUID(text);
        mTodosRelate.append(todo);
        break;

      case ICAL_DTSTART_PROPERTY:
        // Flag that todo has start date. Value is read in by readIncidence().
        todo->setHasStartDate(true);
        break;

      default:
//        kdDebug() << "ICALFormat::readTodo(): Unknown property: " << kind
//                  << endl;
        break;
    }

    p = icalcomponent_get_next_property(vtodo,ICAL_ANY_PROPERTY);
  }

  return todo;
}

Event *ICalFormatImpl::readEvent(icalcomponent *vevent)
{
  Event *event = new Event;
  event->setFloats(false);

  readIncidence(vevent,event);  

  icalproperty *p = icalcomponent_get_first_property(vevent,ICAL_ANY_PROPERTY);

  const char *text;
//  int intvalue;
  icaltimetype icaltime;

  QStringList categories;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_DTEND_PROPERTY:  // start date and time
        icaltime = icalproperty_get_dtend(p);
        if (icaltime.is_date) {
          event->setDtEnd(QDateTime(readICalDate(icaltime),QTime(0,0,0)));
          event->setFloats(true);
        } else {
          event->setDtEnd(readICalDateTime(icaltime));
        }
        break;

// TODO:  
  // at this point, there should be at least a start or end time.
  // fix up for events that take up no time but have a time associated
#if 0
  if (!(vo = isAPropertyOf(vevent, VCDTstartProp)))
    anEvent->setDtStart(anEvent->dtEnd());
  if (!(vo = isAPropertyOf(vevent, VCDTendProp)))
    anEvent->setDtEnd(anEvent->dtStart());
#endif
  
// TODO: exdates
#if 0
  // recurrence exceptions
  if ((vo = isAPropertyOf(vevent, VCExDateProp)) != 0) {
    anEvent->setExDates(s = fakeCString(vObjectUStringZValue(vo)));
    deleteStr(s);
  }
#endif

#if 0
  // secrecy
  if ((vo = isAPropertyOf(vevent, VCClassProp)) != 0) {
    anEvent->setSecrecy(s = fakeCString(vObjectUStringZValue(vo)));
    deleteStr(s);
  }
  else
    anEvent->setSecrecy("PUBLIC");

  // attachments
  tmpStrList.clear();
  initPropIterator(&voi, vevent);
  while (moreIteration(&voi)) {
    vo = nextVObject(&voi);
    if (strcmp(vObjectName(vo), VCAttachProp) == 0) {
      tmpStrList.append(s = fakeCString(vObjectUStringZValue(vo)));
      deleteStr(s);
    }
  }
  anEvent->setAttachments(tmpStrList);

  // resources
  if ((vo = isAPropertyOf(vevent, VCResourcesProp)) != 0) {
    QString resources = (s = fakeCString(vObjectUStringZValue(vo)));
    deleteStr(s);
    tmpStrList.clear();
    index1 = 0;
    index2 = 0;
    QString resource;
    while ((index2 = resources.find(';', index1)) != -1) {
      resource = resources.mid(index1, (index2 - index1));
      tmpStrList.append(resource);
      index1 = index2;
    }
    anEvent->setResources(tmpStrList);
  }
#endif

// TODO: transparency
#if 0
  // transparency
  if ((vo = isAPropertyOf(vevent, VCTranspProp)) != 0) {
    anEvent->setTransparency(atoi(s = fakeCString(vObjectUStringZValue(vo))));
    deleteStr(s);
  }
#endif

      case ICAL_RELATEDTO_PROPERTY:  // releated event (parent)
        text = icalproperty_get_relatedto(p);
        event->setRelatedToVUID(text);
        mEventsRelate.append(event);
        break;

      default:
//        kdDebug() << "ICALFormat::readEvent(): Unknown property: " << kind
//                  << endl;
        break;
    }

    p = icalcomponent_get_next_property(vevent,ICAL_ANY_PROPERTY);
  }

  // some stupid vCal exporters ignore the standard and use Description
  // instead of Summary for the default field.  Correct for this.
  if (event->summary().isEmpty() && 
      !(event->description().isEmpty())) {
    QString tmpStr = event->description().simplifyWhiteSpace();
    event->setDescription("");
    event->setSummary(tmpStr);
  }

  return event;
}

Journal *ICalFormatImpl::readJournal(icalcomponent *vjournal)
{
  Journal *journal = new Journal;

  readIncidence(vjournal,journal);
  
  return journal;  
}

Attendee *ICalFormatImpl::readAttendee(icalproperty *attendee)
{
  Attendee *a;

  QString text = QString::fromUtf8(icalproperty_get_attendee(attendee));
  
  text = text.simplifyWhiteSpace();
  int emailPos1, emailPos2;
  if ((emailPos1 = text.find('<')) > 0) {
    // both email address and name
    emailPos2 = text.find('>');
    a = new Attendee(text.left(emailPos1 - 1),
                     text.mid(emailPos1 + 1, 
                     emailPos2 - (emailPos1 + 1)));
  } else if (text.find('@') > 0) {
    // just an email address
    a = new Attendee(0, text);
  } else {
    // just a name
    a = new Attendee(text);
  }

  icalparameter *p = icalproperty_get_first_parameter(attendee,
                                                      ICAL_RSVP_PARAMETER);
  if (p) {
    a->setRSVP(icalparameter_get_rsvp(p));
  }

// TODO: attendee status
#if 0  
      // is there a status property?
      if ((vp = isAPropertyOf(vo, VCStatusProp)) != 0)
	a->setStatus(vObjectStringZValue(vp));
#endif

  return a;
}

void ICalFormatImpl::readIncidence(icalcomponent *parent,Incidence *incidence)
{
  icalproperty *p = icalcomponent_get_first_property(parent,ICAL_ANY_PROPERTY);

  const char *text;
  int intvalue;
  icaltimetype icaltime;

  QStringList categories;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_CREATED_PROPERTY:
        icaltime = icalproperty_get_created(p);
        incidence->setCreated(readICalDateTime(icaltime));
        break;

      case ICAL_UID_PROPERTY:  // unique id
        text = icalproperty_get_uid(p);
        incidence->setVUID(text);
        break;

      case ICAL_SEQUENCE_PROPERTY:  // sequence
        intvalue = icalproperty_get_sequence(p);
        incidence->setRevision(intvalue);
        break;

      case ICAL_LASTMODIFIED_PROPERTY:  // last modification date
        icaltime = icalproperty_get_lastmodified(p);
        incidence->setLastModified(readICalDateTime(icaltime));
        break;

      case ICAL_ORGANIZER_PROPERTY:  // organizer
        text = icalproperty_get_organizer(p);
        incidence->setOrganizer(QString::fromUtf8(text));
        break;

      case ICAL_ATTENDEE_PROPERTY:  // attendee
        incidence->addAttendee(readAttendee(p));
        break;

      case ICAL_DTSTART_PROPERTY:  // start date and time
        icaltime = icalproperty_get_dtstart(p);
        if (icaltime.is_date) {
          incidence->setDtStart(QDateTime(readICalDate(icaltime),QTime(0,0,0)));
          incidence->setFloats(true);
        } else {
          incidence->setDtStart(readICalDateTime(icaltime));
        }
        break;

      case ICAL_DESCRIPTION_PROPERTY:  // description
        text = icalproperty_get_description(p);
        incidence->setDescription(QString::fromUtf8(text));
        break;
 
      case ICAL_SUMMARY_PROPERTY:  // summary
        text = icalproperty_get_summary(p);
        incidence->setSummary(QString::fromUtf8(text));
        break;

#if 0  
  // status
  if ((vo = isAPropertyOf(vincidence, VCStatusProp)) != 0) {
    incidence->setStatus(s = fakeCString(vObjectUStringZValue(vo)));
    deleteStr(s);
  }
  else
    incidence->setStatus("NEEDS ACTION");
#endif
  
      case ICAL_PRIORITY_PROPERTY:  // priority
        intvalue = icalproperty_get_priority(p);
        incidence->setPriority(intvalue);
        break;

      case ICAL_CATEGORIES_PROPERTY:  // categories
        text = icalproperty_get_categories(p);
        categories.append(QString::fromUtf8(text));
        break;

      case ICAL_RRULE_PROPERTY:
        readRecurrenceRule(p,incidence);
        break;

      case ICAL_EXDATE_PROPERTY:
        icaltime = icalproperty_get_exdate(p);
        incidence->addExDate(readICalDate(icaltime));
        break;
        

      case ICAL_X_PROPERTY:
        if (strcmp(icalproperty_get_name(p),"X-PILOTID") == 0) {
          text = icalproperty_get_value_as_string(p);
          incidence->setPilotId(QString::fromUtf8(text).toInt());
        } else if (strcmp(icalproperty_get_name(p),"X-PILOTSTAT") == 0) {
          text = icalproperty_get_value_as_string(p);
          incidence->setSyncStatus(QString::fromUtf8(text).toInt());
        }
        break;

      default:
//        kdDebug() << "ICALFormat::readIncidence(): Unknown property: " << kind
//                  << endl;
        break;
    }

    p = icalcomponent_get_next_property(parent,ICAL_ANY_PROPERTY);
  }

  // add categories
  incidence->setCategories(categories);

  icalcomponent *alarm;
  alarm = icalcomponent_get_first_component(parent,ICAL_VALARM_COMPONENT);
  if (alarm) {
    readAlarm(alarm,incidence);
  }
}

void ICalFormatImpl::readRecurrenceRule(icalproperty *rrule,Incidence *incidence)
{
  kdDebug() << "Read recurrence for " << incidence->summary() << endl;

  KORecurrence *recur = incidence->recurrence();
  recur->unsetRecurs();

  struct icalrecurrencetype r = icalproperty_get_rrule(rrule);

  dumpIcalRecurrence(r);

  int index = 0;
  short day = 0;
  QBitArray qba(7);

  switch (r.freq) {
    case ICAL_DAILY_RECURRENCE:
      if (!icaltime_is_null_time(r.until)) {
        recur->setDaily(r.interval,readICalDate(r.until));
      } else {
        if (r.count == 0)
          recur->setDaily(r.interval,-1);
        else
          recur->setDaily(r.interval,r.count);
      }
      break;
    case ICAL_WEEKLY_RECURRENCE:
      kdDebug() << "WEEKLY_RECURRENCE" << endl;
      while((day = r.by_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
        kdDebug() << " " << day << endl;
        if (day == 1) qba.setBit(6);
        else qba.setBit(day-2);
      }
      if (!icaltime_is_null_time(r.until)) {
        recur->setWeekly(r.interval,qba,readICalDate(r.until));
      } else {
        if (r.count == 0)
          recur->setWeekly(r.interval,qba,-1);
        else
          recur->setWeekly(r.interval,qba,r.count);
      }
      break;
    case ICAL_MONTHLY_RECURRENCE:
      if (r.by_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
        while((day = r.by_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
          kdDebug() << "----a " << index << ": " << day << endl;
          if (day == 1) qba.setBit(6);
          else qba.setBit(day-2);
        }
        if (!icaltime_is_null_time(r.until)) {
          recur->setMonthly(KORecurrence::rMonthlyPos,r.interval,
                            readICalDate(r.until));
        } else {
          if (r.count == 0)
            recur->setMonthly(KORecurrence::rMonthlyPos,r.interval,-1);
          else
            recur->setMonthly(KORecurrence::rMonthlyPos,r.interval,r.count);
        }
        if (r.by_set_pos[0] != ICAL_RECURRENCE_ARRAY_MAX) {
          recur->addMonthlyPos(r.by_set_pos[0],qba);
        } else {
          recur->addMonthlyPos(0,qba);
        }
      } else if (r.by_month_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
        while((day = r.by_month_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
          kdDebug() << "----b " << day << endl;
          recur->addMonthlyDay(day);
        }
        if (!icaltime_is_null_time(r.until)) {
          recur->setMonthly(KORecurrence::rMonthlyDay,r.interval,
                            readICalDate(r.until));
        } else {
          if (r.count == 0)
            recur->setMonthly(KORecurrence::rMonthlyDay,r.interval,-1);
          else
            recur->setMonthly(KORecurrence::rMonthlyDay,r.interval,r.count);
        }
      }
      break;
    case ICAL_YEARLY_RECURRENCE:
      if (r.by_year_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
        while((day = r.by_year_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
          recur->addYearlyNum(day);
        }
        if (!icaltime_is_null_time(r.until)) {
          recur->setYearly(KORecurrence::rYearlyDay,r.interval,
                            readICalDate(r.until));
        } else {
          if (r.count == 0)
            recur->setYearly(KORecurrence::rYearlyDay,r.interval,-1);
          else
            recur->setYearly(KORecurrence::rYearlyDay,r.interval,r.count);
        }
      } if (r.by_month[0] != ICAL_RECURRENCE_ARRAY_MAX) {
        while((day = r.by_month[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
          recur->addYearlyNum(day);
        }
        if (!icaltime_is_null_time(r.until)) {
          recur->setYearly(KORecurrence::rYearlyMonth,r.interval,
                            readICalDate(r.until));
        } else {
          if (r.count == 0)
            recur->setYearly(KORecurrence::rYearlyMonth,r.interval,-1);
          else
            recur->setYearly(KORecurrence::rYearlyMonth,r.interval,r.count);
        }
      }
      break;
    default:
      kdDebug() << "Unknown type of recurrence: " << r.freq << endl;
      break;
  }

#if 0
  // repeat stuff
  if ((vo = isAPropertyOf(vevent, VCRRuleProp)) != 0) {
    QString tmpStr = (s = fakeCString(vObjectUStringZValue(vo)));
    deleteStr(s);
    tmpStr.simplifyWhiteSpace();
    tmpStr = tmpStr.upper();

    /********************************* DAILY ******************************/
    if (tmpStr.left(1) == "D") {
      int index = tmpStr.find(' ');
      int rFreq = tmpStr.mid(1, (index-1)).toInt();
      index = tmpStr.findRev(' ') + 1; // advance to last field
      if (tmpStr.mid(index,1) == "#") index++;
      if (tmpStr.find('T', index) != -1) {
	QDate rEndDate = (ISOToQDateTime(tmpStr.mid(index, tmpStr.length()-index))).date();
	anEvent->setRecursDaily(rFreq, rEndDate);
      } else {
	int rDuration = tmpStr.mid(index, tmpStr.length()-index).toInt();
	if (rDuration == 0) // VEvents set this to 0 forever, we use -1
	  anEvent->setRecursDaily(rFreq, -1);
	else
	  anEvent->setRecursDaily(rFreq, rDuration);
      }
    } 
    /********************************* WEEKLY ******************************/
    else if (tmpStr.left(1) == "W") {
      int index = tmpStr.find(' ');
      int last = tmpStr.findRev(' ') + 1;
      int rFreq = tmpStr.mid(1, (index-1)).toInt();
      index += 1; // advance to beginning of stuff after freq
      QBitArray qba(7);
      QString dayStr;
      if( index == last ) {
	// e.g. W1 #0
	qba.setBit(anEvent->dtStart().date().dayOfWeek() - 1);
      }
      else {
	// e.g. W1 SU #0
	while (index < last) {
	  dayStr = tmpStr.mid(index, 3);
	  int dayNum = numFromDay(dayStr);
	  qba.setBit(dayNum);
	  index += 3; // advance to next day, or possibly "#"
	}
      }
      index = last; if (tmpStr.mid(index,1) == "#") index++;
      if (tmpStr.find('T', index) != -1) {
	QDate rEndDate = (ISOToQDateTime(tmpStr.mid(index, tmpStr.length()-index))).date();
	anEvent->setRecursWeekly(rFreq, qba, rEndDate);
      } else {
	int rDuration = tmpStr.mid(index, tmpStr.length()-index).toInt();
	if (rDuration == 0)
	  anEvent->setRecursWeekly(rFreq, qba, -1);
	else
	  anEvent->setRecursWeekly(rFreq, qba, rDuration);
      }
    } 
    /**************************** MONTHLY-BY-POS ***************************/
    else if (tmpStr.left(2) == "MP") {
      int index = tmpStr.find(' ');
      int last = tmpStr.findRev(' ') + 1;
      int rFreq = tmpStr.mid(2, (index-1)).toInt();
      index += 1; // advance to beginning of stuff after freq
      QBitArray qba(7);
      short tmpPos;
      if( index == last ) {
	// e.g. MP1 #0
	tmpPos = anEvent->dtStart().date().day()/7 + 1;
	if( tmpPos == 5 )
	  tmpPos = -1;
	qba.setBit(anEvent->dtStart().date().dayOfWeek() - 1);
	anEvent->addRecursMonthlyPos(tmpPos, qba);
      }
      else {
	// e.g. MP1 1+ SU #0
	while (index < last) {
	  tmpPos = tmpStr.mid(index,1).toShort();
	  index += 1;
	  if (tmpStr.mid(index,1) == "-")
	    // convert tmpPos to negative
	    tmpPos = 0 - tmpPos;
	  index += 2; // advance to day(s)
	  while (numFromDay(tmpStr.mid(index,3)) >= 0) {
	    int dayNum = numFromDay(tmpStr.mid(index,3));
	    qba.setBit(dayNum);
	    index += 3; // advance to next day, or possibly pos or "#"
	  }
	  anEvent->addRecursMonthlyPos(tmpPos, qba);
	  qba.detach();
	  qba.fill(FALSE); // clear out
	} // while != "#"
      }
      index = last; if (tmpStr.mid(index,1) == "#") index++;
      if (tmpStr.find('T', index) != -1) {
	QDate rEndDate = (ISOToQDateTime(tmpStr.mid(index, tmpStr.length() - 
						    index))).date();
	anEvent->setRecursMonthly(Event::rMonthlyPos, rFreq, rEndDate);
      } else {
	int rDuration = tmpStr.mid(index, tmpStr.length()-index).toInt();
	if (rDuration == 0)
	  anEvent->setRecursMonthly(Event::rMonthlyPos, rFreq, -1);
	else
	  anEvent->setRecursMonthly(Event::rMonthlyPos, rFreq, rDuration);
      }
    }

    /**************************** MONTHLY-BY-DAY ***************************/
    else if (tmpStr.left(2) == "MD") {
      int index = tmpStr.find(' ');
      int last = tmpStr.findRev(' ') + 1;
      int rFreq = tmpStr.mid(2, (index-1)).toInt();
      index += 1;
      short tmpDay;
      if( index == last ) {
	// e.g. MD1 #0
	tmpDay = anEvent->dtStart().date().day();
	anEvent->addRecursMonthlyDay(tmpDay);
      }
      else {
	// e.g. MD1 3 #0
	while (index < last) {
	  int index2 = tmpStr.find(' ', index); 
	  tmpDay = tmpStr.mid(index, (index2-index)).toShort();
	  index = index2-1;
	  if (tmpStr.mid(index, 1) == "-")
	    tmpDay = 0 - tmpDay;
	  index += 2; // advance the index;
	  anEvent->addRecursMonthlyDay(tmpDay);
	} // while != #
      }
      index = last; if (tmpStr.mid(index,1) == "#") index++;
      if (tmpStr.find('T', index) != -1) {
	QDate rEndDate = (ISOToQDateTime(tmpStr.mid(index, tmpStr.length()-index))).date();
	anEvent->setRecursMonthly(Event::rMonthlyDay, rFreq, rEndDate);
      } else {
	int rDuration = tmpStr.mid(index, tmpStr.length()-index).toInt();
	if (rDuration == 0)
	  anEvent->setRecursMonthly(Event::rMonthlyDay, rFreq, -1);
	else
	  anEvent->setRecursMonthly(Event::rMonthlyDay, rFreq, rDuration);
      }
    }

    /*********************** YEARLY-BY-MONTH *******************************/
    else if (tmpStr.left(2) == "YM") {
      int index = tmpStr.find(' ');
      int last = tmpStr.findRev(' ') + 1;
      int rFreq = tmpStr.mid(2, (index-1)).toInt();
      index += 1;
      short tmpMonth;
      if( index == last ) {
	// e.g. YM1 #0
	tmpMonth = anEvent->dtStart().date().month();
	anEvent->addRecursYearlyNum(tmpMonth);
      }
      else {
	// e.g. YM1 3 #0
	while (index < last) {
	  int index2 = tmpStr.find(' ', index);
	  tmpMonth = tmpStr.mid(index, (index2-index)).toShort();
	  index = index2+1;
	  anEvent->addRecursYearlyNum(tmpMonth);
	} // while != #
      }
      index = last; if (tmpStr.mid(index,1) == "#") index++;
      if (tmpStr.find('T', index) != -1) {
	QDate rEndDate = (ISOToQDateTime(tmpStr.mid(index, tmpStr.length()-index))).date();
	anEvent->setRecursYearly(Event::rYearlyMonth, rFreq, rEndDate);
      } else {
	int rDuration = tmpStr.mid(index, tmpStr.length()-index).toInt();
	if (rDuration == 0)
	  anEvent->setRecursYearly(Event::rYearlyMonth, rFreq, -1);
	else
	  anEvent->setRecursYearly(Event::rYearlyMonth, rFreq, rDuration);
      }
    }

    /*********************** YEARLY-BY-DAY *********************************/
    else if (tmpStr.left(2) == "YD") {
      int index = tmpStr.find(' ');
      int last = tmpStr.findRev(' ') + 1;
      int rFreq = tmpStr.mid(2, (index-1)).toInt();
      index += 1;
      short tmpDay;
      if( index == last ) {
	// e.g. YD1 #0
	tmpDay = anEvent->dtStart().date().dayOfYear();
	anEvent->addRecursYearlyNum(tmpDay);
      }
      else {
	// e.g. YD1 123 #0
	while (index < last) {
	  int index2 = tmpStr.find(' ', index);
	  tmpDay = tmpStr.mid(index, (index2-index)).toShort();
	  index = index2+1;
	  anEvent->addRecursYearlyNum(tmpDay);
	} // while != #
      }
      index = last; if (tmpStr.mid(index,1) == "#") index++;
      if (tmpStr.find('T', index) != -1) {
	QDate rEndDate = (ISOToQDateTime(tmpStr.mid(index, tmpStr.length()-index))).date();
	anEvent->setRecursYearly(Event::rYearlyDay, rFreq, rEndDate);
      } else {
	int rDuration = tmpStr.mid(index, tmpStr.length()-index).toInt();
	if (rDuration == 0)
	  anEvent->setRecursYearly(Event::rYearlyDay, rFreq, -1);
	else
	  anEvent->setRecursYearly(Event::rYearlyDay, rFreq, rDuration);
      }
    } else {
      kdDebug() << "we don't understand this type of recurrence!" << endl;
    } // if
  } // repeats
#endif
}

void ICalFormatImpl::readAlarm(icalcomponent *alarm,Incidence *incidence)
{
  kdDebug() << "Read alarm for " << incidence->summary() << endl;

  KOAlarm* koalarm = incidence->alarm();
  koalarm->setRepeatCount(0);
  koalarm->setEnabled(true);

  icalproperty *p = icalcomponent_get_first_property(alarm,ICAL_ANY_PROPERTY);

  icaltriggertype trigger;
  icalattachtype attach;
  icaldurationtype duration;

  icalproperty_action action;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_ACTION_PROPERTY:
        action = icalproperty_get_action(p);
        break;

      case ICAL_TRIGGER_PROPERTY:
        trigger = icalproperty_get_trigger(p);
        if (icaltime_is_null_time(trigger.time)) {
          kdDebug() << "ICalFormatImpl::readAlarm(): Trigger has no time." << endl;
          if (icaldurationtype_is_null_duration(trigger.duration)) {
            kdDebug() << "ICalFormatImpl::readAlarm(): Trigger has no duration." << endl;
          } else {
            kdDebug() << "ICalFormatImpl::readAlarm(): Trigger has duration." << endl;
          }
        } else {
          koalarm->setTime(readICalDateTime(trigger.time));
        }
        break;

      case ICAL_DURATION_PROPERTY:
        duration = icalproperty_get_duration(p);
        koalarm->setSnoozeTime(icaldurationtype_as_int(duration)/60);
        break;

      case ICAL_REPEAT_PROPERTY:
        koalarm->setRepeatCount(icalproperty_get_repeat(p));
        break;

      // Only in AUDIO and PROCEDURE alarms
      case ICAL_ATTACH_PROPERTY:
        // TODO: move reading the attachement after reading the action type
        attach = icalproperty_get_attach(p);
        koalarm->setAudioFile(QFile::decodeName(
            icalattachtype_get_url(&attach)));
        break;
  
      // Only in DISPLAY and EMAIL and PROCEDURE alarms
      case ICAL_DESCRIPTION_PROPERTY:
        koalarm->setText(icalproperty_get_description(p));
        break;

      // Only in EMAIL alarm
      case ICAL_SUMMARY_PROPERTY:
        koalarm->setMailSubject(icalproperty_get_summary(p));
        break;

      // Only in EMAIL alarm
      case ICAL_ATTENDEE_PROPERTY:
        koalarm->setMailAddress(icalproperty_get_attendee(p));
        // TODO: multiple attendees can be specified
        break;
  
      default:
        break;
    }

    p = icalcomponent_get_next_property(alarm,ICAL_ANY_PROPERTY);
  }

  // TODO: check for consistency of alarm properties

// TODO: read alarms
#if 0
  /* alarm stuff */
  if ((vo = isAPropertyOf(vevent, VCDAlarmProp))) {
    VObject *a;
    if ((a = isAPropertyOf(vo, VCRunTimeProp))) {
      anEvent->setTime(ISOToQDateTime(s = fakeCString(vObjectUStringZValue(a))));
      deleteStr(s);
    }
    anEvent->setEnabled(true);
    if ((vo = isAPropertyOf(vevent, VCPAlarmProp))) {
      if ((a = isAPropertyOf(vo, VCProcedureNameProp))) {
	anEvent->setProgramFile(s = fakeCString(vObjectUStringZValue(a)));
	deleteStr(s);
      }
    }
    if ((vo = isAPropertyOf(vevent, VCAAlarmProp))) {
      if ((a = isAPropertyOf(vo, VCAudioContentProp))) {
	anEvent->setAudioFile(s = fakeCString(vObjectUStringZValue(a)));
	deleteStr(s);
      }
    }
  }
#endif
}

icaltimetype ICalFormatImpl::writeICalDate(const QDate &date)
{
  icaltimetype t;
  
  t.year = date.year();
  t.month = date.month();
  t.day = date.day();

  t.hour = 0;
  t.minute = 0;
  t.second = 0;

  t.is_date = 1;
  
  t.is_utc = 0;

  t.zone = 0;

  return t;
}

icaltimetype ICalFormatImpl::writeICalDateTime(const QDateTime &datetime)
{
  icaltimetype t;
  
  t.year = datetime.date().year();
  t.month = datetime.date().month();
  t.day = datetime.date().day();

  t.hour = datetime.time().hour();
  t.minute = datetime.time().minute();
  t.second = datetime.time().second();

  t.is_date = 0;

  t.is_utc = 0;
  
  t.zone = 0;
  
  return t;
}

QDateTime ICalFormatImpl::readICalDateTime(icaltimetype t)
{
/*
  kdDebug() << "ICalFormatImpl::readICalDateTime()" << endl;
  kdDebug() << "--- Y: " << t.year << " M: " << t.month << " D: " << t.day
            << endl;
  kdDebug() << "--- H: " << t.hour << " M: " << t.minute << " S: " << t.second
            << endl;
  kdDebug() << "--- isDate: " << t.is_date << endl;
*/

  return QDateTime(QDate(t.year,t.month,t.day),
                   QTime(t.hour,t.minute,t.second));
}

QDate ICalFormatImpl::readICalDate(icaltimetype t)
{
  return QDate(t.year,t.month,t.day);
}

icalcomponent *ICalFormatImpl::createCalendarComponent()
{
  icalcomponent *calendar;

  // Root component
  calendar = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);

  icalproperty *p;
  
  // Product Identifier
  p = icalproperty_new_prodid(const_cast<char *>(_PRODUCT_ID));
  icalcomponent_add_property(calendar,p);
  
  // TODO: Add time zone
  
  // iCalendar version (2.0)
  p = icalproperty_new_version(const_cast<char *>(_ICAL_VERSION));
  icalcomponent_add_property(calendar,p);
  
  return calendar;
}



// take a raw vcalendar (i.e. from a file on disk, clipboard, etc. etc.
// and break it down from it's tree-like format into the dictionary format
// that is used internally in the ICalFormatImpl.
bool ICalFormatImpl::populate(icalcomponent *calendar)
{
  // this function will populate the caldict dictionary and other event 
  // lists. It turns vevents into Events and then inserts them.

    if (!calendar) return false;

// TODO: check for METHOD
#if 0
  if ((curVO = isAPropertyOf(vcal, ICMethodProp)) != 0) {
    char *methodType = 0;
    methodType = fakeCString(vObjectUStringZValue(curVO));
    if (mEnableDialogs)
      KMessageBox::information(mTopWidget, 
			       i18n("This calendar is an iTIP transaction of type \"%1\".")
			       .arg(methodType),
                               i18n("KOrganizer: iTIP Transaction"));
    delete methodType;
  }
#endif

// TODO: check for unknown PRODID
#if 0
  // warn the user that we might have trouble reading non-known calendar.
  if ((curVO = isAPropertyOf(vcal, VCProdIdProp)) != 0) {
    char *s = fakeCString(vObjectUStringZValue(curVO));
    if (strcmp(_PRODUCT_ID, s) != 0)
      if (mEnableDialogs)
	KMessageBox::information(mTopWidget, 
			     i18n("This vCalendar file was not created by KOrganizer\n"
				     "or any other product we support. Loading anyway..."),
                             i18n("KOrganizer: Unknown vCalendar Vendor"));
    deleteStr(s);
  }
#endif

  icalproperty *p;
  
  p = icalcomponent_get_first_property(calendar,ICAL_VERSION_PROPERTY);
  if (!p) {
    kdDebug() << "No VERSION property found" << endl;
    return false;
  } else {
    const char *version = icalproperty_get_version(p);
    kdDebug() << "VCALENDAR version: '" << version << "'" << endl; 
    
    if (strcmp(version,"1.0") == 0) {
      kdDebug() << "Expected iCalendar, got vCalendar" << endl;
      mParent->setException(new KOErrorFormat(KOErrorFormat::CalVersion1,
                            i18n("Expected iCalendar format")));
      return false;
    } else if (strcmp(version,"2.0") != 0) {
      kdDebug() << "Expected iCalendar, got unknown format" << endl;
      mParent->setException(new KOErrorFormat(KOErrorFormat::CalVersionUnknown));
      return false;
    }
  }


// TODO: check for calendar format version
#if 0  
  // warn the user we might have trouble reading this unknown version.
  if ((curVO = isAPropertyOf(vcal, VCVersionProp)) != 0) {
    char *s = fakeCString(vObjectUStringZValue(curVO));
    if (strcmp(_VCAL_VERSION, s) != 0)
      if (mEnableDialogs)
	KMessageBox::sorry(mTopWidget, 
			     i18n("This vCalendar file has version %1.\n"
			          "We only support %2.")
                             .arg(s).arg(_VCAL_VERSION),
                             i18n("KOrganizer: Unknown vCalendar Version"));
    deleteStr(s);
  }
#endif

// TODO: set time zone
#if 0  
  // set the time zone
  if ((curVO = isAPropertyOf(vcal, VCTimeZoneProp)) != 0) {
    char *s = fakeCString(vObjectUStringZValue(curVO));
    mCalendar->setTimeZone(s);
    deleteStr(s);
  }
#endif

  // Store all events with a relatedTo property in a list for post-processing
  mEventsRelate.clear();
  mTodosRelate.clear();
  // TODO: make sure that only actually added ecvens go to this lists.

  icalcomponent *c;

  // Iterate through all todos
  c = icalcomponent_get_first_component(calendar,ICAL_VTODO_COMPONENT);
  while (c) {
    kdDebug() << "----Todo found" << endl;
    Todo *todo = readTodo(c);
    if (!mCalendar->getTodo(todo->VUID())) mCalendar->addTodo(todo);
    c = icalcomponent_get_next_component(calendar,ICAL_VTODO_COMPONENT);
  }
  
  // Iterate through all events
  c = icalcomponent_get_first_component(calendar,ICAL_VEVENT_COMPONENT);
  while (c) {
    kdDebug() << "----Event found" << endl;  
    Event *event = readEvent(c);
    if (!mCalendar->getEvent(event->VUID())) mCalendar->addEvent(event);
    c = icalcomponent_get_next_component(calendar,ICAL_VEVENT_COMPONENT);
  }
  
  // Iterate through all journals
  c = icalcomponent_get_first_component(calendar,ICAL_VJOURNAL_COMPONENT);
  while (c) {
    kdDebug() << "----Journal found" << endl;  
    Journal *journal = readJournal(c);
    if (!mCalendar->journal(journal->VUID())) mCalendar->addJournal(journal);
    c = icalcomponent_get_next_component(calendar,ICAL_VJOURNAL_COMPONENT);
  }
  
#if 0
  initPropIterator(&i, vcal);

  // go through all the vobjects in the vcal
  while (moreIteration(&i)) {
    curVO = nextVObject(&i);

    /************************************************************************/

    // now, check to see that the object is an event or todo.
    if (strcmp(vObjectName(curVO), VCEventProp) == 0) {

      if ((curVOProp = isAPropertyOf(curVO, KPilotStatusProp)) != 0) {
	char *s;
	s = fakeCString(vObjectUStringZValue(curVOProp));
	// check to see if event was deleted by the kpilot conduit
	if (atoi(s) == Event::SYNCDEL) {
	  deleteStr(s);
	  kdDebug() << "skipping pilot-deleted event" << endl;
	  goto SKIP;
	}
	deleteStr(s);
      }

      // this code checks to see if we are trying to read in an event
      // that we already find to be in the calendar.  If we find this
      // to be the case, we skip the event.
      if ((curVOProp = isAPropertyOf(curVO, VCUniqueStringProp)) != 0) {
	char *s = fakeCString(vObjectUStringZValue(curVOProp));
	QString tmpStr(s);
	deleteStr(s);

	if (mCalendar->getEvent(tmpStr)) {
	  goto SKIP;
	}
	if (mCalendar->getTodo(tmpStr)) {
	  goto SKIP;
	}
      }

      if ((!(curVOProp = isAPropertyOf(curVO, VCDTstartProp))) &&
	  (!(curVOProp = isAPropertyOf(curVO, VCDTendProp)))) {
	kdDebug() << "found a VEvent with no DTSTART and no DTEND! Skipping..." << endl;
	goto SKIP;
      }

      anEvent = VEventToEvent(curVO);
      // we now use addEvent instead of insertEvent so that the
      // signal/slot get connected.
      if (anEvent)
	mCalendar->addEvent(anEvent);
      else {
	// some sort of error must have occurred while in translation.
	goto SKIP;
      }
    } else if (strcmp(vObjectName(curVO), VCTodoProp) == 0) {
      anEvent = VTodoToEvent(curVO);
      mCalendar->addTodo(anEvent);
    } else if ((strcmp(vObjectName(curVO), VCVersionProp) == 0) ||
	       (strcmp(vObjectName(curVO), VCProdIdProp) == 0) ||
	       (strcmp(vObjectName(curVO), VCTimeZoneProp) == 0)) {
      // do nothing, we know these properties and we want to skip them.
      // we have either already processed them or are ignoring them.
      ;
    } else {
      kdDebug() << "Ignoring unknown vObject \"" << vObjectName(curVO) << "\"" << endl;
    }
  SKIP:
    ;
  } // while
#endif
  
  // Post-Process list of events with relations, put Event objects in relation
  Event *ev;
  for ( ev=mEventsRelate.first(); ev != 0; ev=mEventsRelate.next() ) {
    ev->setRelatedTo(mCalendar->getEvent(ev->relatedToVUID()));
  }
  Todo *todo;
  for ( todo=mTodosRelate.first(); todo != 0; todo=mTodosRelate.next() ) {
    todo->setRelatedTo(mCalendar->getTodo(todo->relatedToVUID()));
  }

  return true;
}

QString ICalFormatImpl::extractErrorProperty(icalcomponent *c)
{
//  kdDebug() << "ICalFormatImpl:extractErrorProperty: "
//            << icalcomponent_as_ical_string(c) << endl;

  QString errorMessage;

  icalproperty *error;
  error = icalcomponent_get_first_property(c,ICAL_XLICERROR_PROPERTY);
  while(error) {
    errorMessage += icalproperty_get_xlicerror(error);
    errorMessage += "\n";
    error = icalcomponent_get_next_property(c,ICAL_XLICERROR_PROPERTY);
  }

//  kdDebug() << "ICalFormatImpl:extractErrorProperty: " << errorMessage << endl;
  
  return errorMessage;
}

void ICalFormatImpl::dumpIcalRecurrence(icalrecurrencetype r)
{
  int i;

  kdDebug() << " Freq: " << r.freq << endl;
  kdDebug() << " Until: " << icaltime_as_ctime(r.until) << endl;
  kdDebug() << " Count: " << r.count << endl;
  if (r.by_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Day: ";
    while((i = r.by_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + " ");
    }
    kdDebug() << out << endl;
  }
  if (r.by_month_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Month Day: ";
    while((i = r.by_month_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + " ");
    }
    kdDebug() << out << endl;
  }
  if (r.by_year_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Year Day: ";
    while((i = r.by_year_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + " ");
    }
    kdDebug() << out << endl;
  }
  if (r.by_month[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Month: ";
    while((i = r.by_month[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + " ");
    }
    kdDebug() << out << endl;
  }
  if (r.by_set_pos[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Set Pos: ";
    while((i = r.by_set_pos[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      kdDebug() << "========= " << i << endl;
      out.append(QString::number(i) + " ");
    }
    kdDebug() << out << endl;
  }
}

icalcomponent *ICalFormatImpl::createScheduleComponent(Incidence *incidence,
                                                   Scheduler::Method method)
{
  icalcomponent *message = createCalendarComponent();

  icalproperty_method icalmethod = ICAL_METHOD_NONE;
  
  switch (method) {
    case Scheduler::Publish:
      icalmethod = ICAL_METHOD_PUBLISH;
      break;
    case Scheduler::Request:
      icalmethod = ICAL_METHOD_REQUEST;
      break;
    case Scheduler::Refresh:
      icalmethod = ICAL_METHOD_REFRESH;
      break;
    case Scheduler::Cancel:
      icalmethod = ICAL_METHOD_CANCEL;
      break;
    case Scheduler::Add:
      icalmethod = ICAL_METHOD_ADD;
      break;
    case Scheduler::Reply:
      icalmethod = ICAL_METHOD_REPLY;
      break;
    case Scheduler::Counter:
      icalmethod = ICAL_METHOD_COUNTER;
      break;
    case Scheduler::Declinecounter:
      icalmethod = ICAL_METHOD_DECLINECOUNTER;
      break;
    default:
      kdDebug() << "ICalFormat::createScheduleMessage(): Unknow method" << endl;
      return message;
  }

  icalcomponent_add_property(message,icalproperty_new_method(icalmethod));

  // TODO: check, if dynamic cast is required
  Todo *todo = dynamic_cast<Todo *>(incidence);
  if (todo) {
    icalcomponent_add_component(message,writeTodo(todo));
  }
  Event *event = dynamic_cast<Event *>(incidence);
  if (event) {
    icalcomponent_add_component(message,writeEvent(event));
  }

  return message;
}
