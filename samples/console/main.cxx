/*
 * main.cxx
 *
 * OPAL application source file for console mode OPAL videophone
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include "precompile.h"
#include "main.h"


extern const char Manufacturer[] = "Vox Gratia";
extern const char Application[] = "OPAL Console";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);

// Debug command: -m 123 -V -S udp$*:25060 -H tcp$*:21720 -ttttodebugstream


MyManager::MyManager()
  : OpalManagerCLI(OPAL_CONSOLE_PREFIXES OPAL_PREFIX_PCSS)
  , m_autoAnswerMode(NoAutoAnswer)
{
  m_autoAnswerTimer.SetNotifier(PCREATE_NOTIFIER(AutoAnswer));
}


PString MyManager::GetArgumentSpec() const
{
  PString spec = OpalManagerCLI::GetArgumentSpec();

  return "[Application options:]"
         "a-auto-answer;    Automatically answer incoming calls.\n"
         + spec;
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm, "[ <options> ... ]\n[ <options> ... ] <remote-URI>\n[ <options> ... ] <local-URI> <remote-URI>");
}


bool MyManager::Initialise(PArgList & args, bool verbose)
{
  if (!OpalManagerCLI::Initialise(args, verbose, OPAL_PREFIX_PCSS":"))
    return false;

  SetAutoAnswer(LockedStream(*this), verbose, args, "auto-answer");

  m_cli->SetPrompt("OPAL> ");
  m_cli->SetCommand("speed-dial",  PCREATE_NOTIFIER(CmdSpeedDial), "Set speed dial", "[ <name> [ <url> ] ]");
  m_cli->SetCommand("auto-answer", PCREATE_NOTIFIER(CmdAutoAnswer), "Answer call automatically", "\"off\" | \"refuse\" | \"immediate\" | <seconds>");
  m_cli->SetCommand("answer",      PCREATE_NOTIFIER(CmdAnswer),   "Answer call");

  switch (args.GetCount()) {
    case 0 :
      break;

    case 1 :
      if (SetUpCall(OPAL_PREFIX_PCSS":*", args[0]) != NULL)
        *LockedStream(*this) << "Starting call to " << args[0] << endl;
      break;

    case 2 :
      if (SetUpCall(args[0], args[1]) != NULL)
        *LockedStream(*this) << "Starting call from " << args[0] << " to " << args[1] << endl;
      break;

    default :
      Usage(cerr, args);
      return false;
  }

  return true;
}


bool MyManager::OnLocalIncomingCall(OpalLocalConnection & connection)
{
  PStringStream output;
  output << '\n' << connection.GetCall().GetToken() << ": incoming call at "
         << PTime().AsString("w h : mma") << " from " << connection.GetCall().GetPartyA();

  if (!OpalManagerCLI::OnLocalIncomingCall(connection)) {
    output << " rejected.";
    Broadcast(output);
    return false;
  }

  // If in a call and auto answer is on, we are busy
  if (m_incomingCall != NULL) {
    output << " refused as busy.";
    Broadcast(output);
    return false;
  }

  switch (m_autoAnswerMode) {
    case NoAutoAnswer :
      output << ", answer? ";
      break;
    case AutoAnswerImmediate :
      output << " immediately.";
      connection.AcceptIncoming();
      break;
    case AutoAnswerDelayed :
      output << " in " << m_autoAnswerTime.GetSeconds() << " seconds.";
      m_autoAnswerTimer = m_autoAnswerTime;
      m_incomingCall = &connection.GetCall();
      break;
    case AutoAnswerRefuse :
      output << " refused.";
      Broadcast(output);
      return false;
  }

  Broadcast(output);
  return true;
}


void MyManager::AutoAnswer(PTimer &, P_INT_PTR)
{
  if (m_incomingCall == NULL)
    return;

  PSafePtr<OpalLocalConnection> connection = m_incomingCall->GetConnectionAs<OpalLocalConnection>();
  if (connection == NULL)
    return;

  connection->AcceptIncoming();
  m_incomingCall.SetNULL();
}


void MyManager::OnClearedCall(OpalCall & call)
{
  if (m_incomingCall.GetObject() == &call)
    m_incomingCall.SetNULL();

  OpalManagerCLI::OnClearedCall(call);
}


bool MyManager::SetAutoAnswer(ostream & output, bool verbose, const PArgList & args, const char * option)
{
  if (option != NULL ? args.HasOption(option) : (args.GetCount() > 0)) {
    PCaselessString value = option != NULL ? args.GetOptionString(option) : args[0];
    if (value == "off")
      m_autoAnswerMode = NoAutoAnswer;
    else if (value == "on" || value == "immediate")
      m_autoAnswerMode = AutoAnswerImmediate;
    else if (value == "refuse")
      m_autoAnswerMode = AutoAnswerRefuse;
    else if (value.FindSpan("0123456789") == P_MAX_INDEX) {
      m_autoAnswerMode = AutoAnswerDelayed;
      m_autoAnswerTime.SetInterval(0, value.AsInteger());
    }
    else
      return false;
  }

  FindEndPointAs<OpalPCSSEndPoint>(OPAL_PREFIX_PCSS)->SetDeferredAnswer(m_autoAnswerTime != 0);

  if (verbose) {
    output << "Auto answer: ";
    switch (m_autoAnswerMode) {
      case NoAutoAnswer :
        output << "off";
        break;
      case AutoAnswerImmediate :
        output << "immediate";
        break;
      case AutoAnswerDelayed :
        output << setprecision(0) << m_autoAnswerTime << " seconds";
        break;
      case AutoAnswerRefuse :
        output << "refuse";
        break;
    }
    output << endl;
  }

  return true;
}


void MyManager::CmdAutoAnswer(PCLI::Arguments & args, P_INT_PTR)
{
  if (!SetAutoAnswer(args.GetContext(), true, args, NULL))
    args.WriteError("Invalid value for seconds");
}


void MyManager::CmdAnswer(PCLI::Arguments & args, P_INT_PTR)
{
  if (m_incomingCall == NULL)
    args.WriteError() << "No call to answer." << endl;
  else if (m_incomingCall->IsEstablished())
    args.WriteError() << "Call already answered." << endl;
  else {
    PSafePtr<OpalLocalConnection> connection = m_incomingCall->GetConnectionAs<OpalLocalConnection>();
    if (connection == NULL)
      args.WriteError() << "Call has disappeared." << endl;
    else {
      connection->AcceptIncoming();
      args.GetContext() << "Answered call from " << m_incomingCall->GetPartyA() << endl;
    }
  }
}


void MyManager::CmdSpeedDial(PCLI::Arguments & args, P_INT_PTR)
{
  PStringToString::iterator it;
  switch (args.GetCount()) {
    case 0 :
      for (it = m_speedDial.begin(); it != m_speedDial.end(); ++it)
        args.GetContext() << it->first << " -> " << it->second << endl;
      break;

    case 1 :
      if ((it = m_speedDial.find(args[0])) == m_speedDial.end())
        args.WriteError("No speed dial of that name.");
      else
        args.GetContext() << it->first << " -> " << it->second << endl;
      break;

    default :
      m_speedDial.SetAt(args[0], args.GetParameters(1).ToString());
      args.GetContext() << args[0] << " -> " << m_speedDial[args[0]] << endl;
  }
}


void MyManager::AdjustCmdCallArguments(PString & from, PString & to)
{
  if (from.IsEmpty())
    from = OPAL_PREFIX_PCSS":*";

  if (m_speedDial.Contains(to))
    to = m_speedDial[to];
}


// End of File ///////////////////////////////////////////////////////////////
