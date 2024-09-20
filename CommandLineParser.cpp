#include "CommandLineParser.h"


CommandLineParser::CommandLineParser()
{
}


void CommandLineParser::process(const QStringList &arguments)
{
    // オプション解析
    for (const QString &arg : arguments) {
        if (arg == "--help" || arg == "-h") {
            m_HelpSet = true;
        }
        else if (arg == "--version" || arg == "-v") {
            m_VersionSet = true;
        }
        else if (arg.startsWith("--sysconf=")) {
            m_SysConfSet  = true;
        }
        else if (arg.startsWith("-")) {
            // 未知のオプションとして扱う
            m_unknownOptionNames.append(arg);
        }
    }

    // 標準の解析を実行
    QCommandLineParser::process(arguments);
}


QStringList CommandLineParser::unknownOptionNames() const
{
    return m_unknownOptionNames;
}


bool CommandLineParser::isHelpSet() const
{
    return m_HelpSet;
}


bool CommandLineParser::isVersionSet() const
{
    return m_VersionSet;
}


bool CommandLineParser::isSysConfSet() const
{
    return m_SysConfSet;
}
