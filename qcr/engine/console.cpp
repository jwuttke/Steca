//  ***********************************************************************************************
//
//  libqcr: capture and replay Qt widget actions
//
//! @file      qcr/engine/console.cpp
//! @brief     Implements class Console
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2018-
//! @author    Joachim Wuttke
//
//  ***********************************************************************************************

#include <regex>
#include <iostream>
#include <QString>

namespace {

//! Parses a command line, sets the command and the context, and returns true if successful.

//! The input line may be either a plain command or a log entry.
//! A log entry starts with a '[..]' comment containing execution time (optional) and context.
//! It may end with a '#..' comment.
//!
//! Covered by utest/qcr/local/10_console

bool parseCommandLine(const QString& line, QString& command, QString& context)
{
    const std::regex my_regex("^(\\[\\s*((\\d+)ms)?\\s*(\\w+)\\s\\w{3}\\])?([^#]*)(#.*)?$");
    std::smatch my_match;
    const std::string tmpLine { line.toLatin1().constData() };
    if (!std::regex_match(tmpLine, my_match, my_regex))
        return false;
    if (my_match.size()!=7) {
        std::cerr << "BUG: invalid match size\n";
        exit(-1);
    }
    context = QString{my_match[4].str().c_str()};
    command = QString{my_match[5].str().c_str()}.trimmed();
    return true;
}

} // namespace

#ifndef LOCAL_CODE_ONLY

#include "qcr/engine/console.h"
#include "qcr/engine/mixin.h"
#include "qcr/base/qcrexception.h"
#include "qcr/base/string_ops.h"
#include "qcr/base/debug.h" // ASSERT
#include <QApplication>
#include <QFile>

#ifdef Q_OS_WIN
#include <QWinEventNotifier>
#include <windows.h>
#else
#include <QSocketNotifier>
#endif

Console* gConsole; //!< global

QTextStream qterr(stderr);

//  ***********************************************************************************************
//! @class CommandRegistry

//! Holds console (= terminal = command-line) commands to be defined and executed by class Console.
class CommandRegistry {
public:
    CommandRegistry() = delete;
    CommandRegistry(const CommandRegistry&) = delete;
    CommandRegistry(const QString& _name) : name_{_name} {}
    QString learn(const QString&, QcrCommandable*);
    void forget(const QString&);
    QcrCommandable* find(const QString& name);
    void dump(QTextStream&) const;
    QString name() const { return name_; }
    int size() const { return widgets_.size(); }
private:
    const QString name_;
    std::map<const QString, QcrCommandable*> widgets_;
    std::map<const QString, int> numberedEntries_;
};

QString CommandRegistry::learn(const QString& name, QcrCommandable* widget)
{
    ASSERT(name!=""); // empty name only allowed for non-settable QcrBase
    // qterr << "Registry " << name_ << " learns '" << name << "'\n"; qterr.flush();
    QString ret = name;
    if (ret.contains("#")) {
        auto numberedEntry = numberedEntries_.find(name);
        int idxEntry = 1;
        if (numberedEntry==numberedEntries_.end())
            numberedEntries_[name] = idxEntry;
        else
            idxEntry = ++(numberedEntry->second);
        ret.replace("#", QString::number(idxEntry));
    }
    if (widgets_.find(ret)!=widgets_.end()) {
        QByteArray tmp = ret.toLatin1();
        qFatal("Duplicate widget registry entry '%s'", tmp.constData());
    }
    widgets_[ret] = widget;
    return ret;
}

void CommandRegistry::forget(const QString& name)
{
    // qterr << "Registry " << name_ << "(" << widgets_.size() << ") forgets '"  << name << "'\n";
    // qterr.flush();
    auto it = widgets_.find(name);
    if (it==widgets_.end()) {
        QByteArray tmp1 = name.toLatin1();
        QByteArray tmp2 = name_.toLatin1();
        qFatal("Cannot deregister, there is no entry '%s' in the widget registry '%s'",
               tmp1.constData(), tmp2.constData());
    }
    widgets_.erase(it);
}

QcrCommandable* CommandRegistry::find(const QString& name)
{
    auto entry = widgets_.find(name);
    if (entry==widgets_.end())
        return {};
    return entry->second;
}

void CommandRegistry::dump(QTextStream& stream) const
{
    for (auto it: widgets_)
        stream << " " << it.first;
    stream << "\n";
}

//  ***********************************************************************************************
//! @class Console

Console::Console(const QString& logFileName)
{
    gConsole = this;

#ifdef Q_OS_WIN
    notifier_ = new QWinEventNotifier;// GetStdHandle(STD_INPUT_HANDLE));
    QObject::connect(notifier_, &QWinEventNotifier::activated, [this](HANDLE){ readCLI(); });
#else
    notifier_ = new QSocketNotifier{fileno(stdin), QSocketNotifier::Read};
    QObject::connect(notifier_, &QSocketNotifier::activated, [this](int){ readCLI(); });
#endif

    // start registry
    registryStack_.push(new CommandRegistry{"main"});

    // start log
    auto* file = new QFile{logFileName};
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        qFatal("cannot open log file");
    log_.setDevice(file);
    startTime_ = QDateTime::currentDateTime();
    caller_ = "log";
    log("# " + qApp->applicationName() + " " + qApp->applicationVersion() + " started at "
        + startTime_.toString("yyyy-MM-dd HH:mm::ss.zzz"));
    caller_ = "ini";
}

Console::~Console()
{
    caller_ = "log";
    log("# " + qApp->applicationName() + " session ended");
    log("# duration: " + QString::number(startTime_.msecsTo(QDateTime::currentDateTime())) + "ms");
    log("# computing time: " + QString::number(computingTime_) + "ms");
    delete log_.device();
    while (!registryStack_.empty()) {
        delete registryStack_.top();
        registryStack_.pop();
    }
    gConsole = nullptr;
}

//! Registers a QcrCommandable or pushes a new registry; returns registered name.

//! The registered name will deviated from the name passed as argument if it contains
//! a "#" (which will be replaced by a unique number).
//!
//! The registry will be used in "wrappedCommand" which forwards a command to the
//! QcrCommandable which then executes it.
//!
//! In the special case of nameArg="@push <name>", a new registry is pushed to current.
//! This is used by the QcrModal modal dialogs. On terminating, QcrModal calls
//! closeModalDialog(), which pops the current registry away, so that the previous
//! registry is reinstated.
QString Console::learn(const QString& nameArg, QcrCommandable* widget)
{
    QString name = nameArg;
    if (name[0]=='@') {
        QStringList args = name.split(' ');
        if (args[0]!="@push") {
            QByteArray tmp = name.toLatin1();
            qFatal("invalid @ command in learn(%s)", tmp.constData());
        }
        if (args.size()<2) {
            QByteArray tmp = name.toLatin1();
            qFatal("@push has no argument in learn(%s)", tmp.constData());
        }
        name = args[1];
        registryStack_.push(new CommandRegistry{name});
        qDebug() << "pushed registry " << registryStack_.top()->name();
    }
    return registry().learn(name, widget);
}

//! Unregisters a QcrCommandable.
void Console::forget(const QString& name)
{
    qDebug() << "forget " << name;
    registry().forget(name);
}

//! Sets calling context to GUI. To be called when initializations are done.
void Console::startingGui()
{
    caller_ = "gui";
}

//! Reads and executes a command script.
void Console::runScript(const QString& fName)
{
    QFile file(fName);
    log("# running script '" + fName + "'");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file " << fName;
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line[0]=='[') {
            int i = line.indexOf(']');
            if (i==-1) {
                qterr << "unbalanced '['\n"; qterr.flush();
                return;
            }
            line = line.mid(i+1);
        }
        commandLifo_.push_back(line);
    }
    commandsFromStack();
    log("# done with script '" + fName + "'");
}

//! Pops the current registry away, so that the previous one is reinstated.

//! Called by ~QcrModal(), i.e. on terminating a modal dialog.
void Console::closeModalDialog()
{
    log(registryStack_.top()->name() + " close");
    if (registryStack_.empty())
        qFatal("BUG: cannot pop: registry stack is empty");
    qDebug() << "going to pop registry " << registryStack_.top()->name();
    delete registryStack_.top();
    registryStack_.pop();
    qDebug() << "top registry is now " << registryStack_.top()->name();
}

//! Executes commands on stack. Called by runScript and by QcrModalDialog/QcrFileDialog::exec.
void Console::commandsFromStack()
{
    while (!commandLifo_.empty()) {
        const QString line = commandLifo_.front();
        qDebug() << "/from stack '" << line;
        commandLifo_.pop_front();
        try {
            commandInContext(line, "fil");
        } catch (const QcrException&ex) {
            qterr << ex.msg() << "\n";
            qterr.flush();
            commandLifo_.clear();
            log("# ERROR: " + ex.msg());
            log("# Emptied command stack upon error");
            break;
        }
        qDebug() << "from stack/ '" << line << "'\n";
    }
}

//! Writes line to log file, decorated with information on context and timing.
void Console::log(const QString& lineArg) const
{
    QString line = lineArg;
    static auto lastTime = startTime_;
    const auto currTime = QDateTime::currentDateTime();
    int tDiff = lastTime.msecsTo(currTime);
    lastTime = currTime;
    QString prefix = "[";
    if (caller_=="gui" && line[0]!='#') {
        prefix += "       "; // direct user action: we don't care how long the user was idle
    } else {
        prefix += QString::number(tDiff).rightJustified(5) + "ms";
        computingTime_ += tDiff;
    }
    prefix += " " + registry().name() + " " + caller_ + "] ";
    log_ << prefix << line << "\n";
    log_.flush();
    // also write to terminal ?
    if (line.indexOf("##")==0)
        return; // this line has already been written to terminal by our messageHandler
    qterr << line << "\n"; qterr.flush();
}

//! Reads one line from the command-line interface, and executes it.
void Console::readCLI()
{
    QTextStream qtin(stdin);
    QString line = qtin.readLine();
    qDebug() << "readCLI: " << line;
    try {
        commandInContext(line, "cli");
    } catch (const QcrException&ex) {
        qterr << ex.msg() << "\n";
        qterr.flush();
    }
}

//! Delegates command execution to wrappedCommand, with context set to caller argument.
void Console::commandInContext(const QString& line, const QString& caller)
{
    qDebug() << "/in context '" << line << "', caller=" << caller;
    caller_ = caller;
    wrappedCommand(line);
    caller_ = "gui"; // restores default
    qDebug() << "in context/ '" << line << "', caller=" << caller;
}

//! Executes command. Always called from commandInContext(..).

//! Commands are either console commands (starting with '@'), or widget commands.
//! Widget commands start with the name of widget that has been registered by learn(..);
//! further execution is delegated to the pertinent widget.
void Console::wrappedCommand(const QString& line)
{
    QString command, context;
    if (!parseCommandLine(line, command, context))
        throw QcrException{"Command line '"+line+"' could not be parsed"};
    if (command=="")
        return;
    QString cmd, arg;
    strOp::splitOnce(command, cmd, arg);
    qDebug() << "/wrapped '" << line;
    if (cmd=="@ls") {
        const CommandRegistry* reg = registryStack_.top();
        qterr << "registry " << reg->name() << " has " << reg->size() << " commands:\n";
        reg->dump(qterr);
        qterr.flush();
        return;
    }
    QcrCommandable* w = registry().find(cmd);
    if (!w)
        throw QcrException{"Command '"+cmd+"' not found"};
    try {
        w->setFromCommand(arg); // execute command
    } catch (const QcrException&ex) {
        throw QcrException{"Command '"+cmd+"' failed: "+ex.msg()};
    }
    qDebug() << "wrapped/ '" << line << "'";
}

#endif // LOCAL_CODE_ONLY
