/*
 * Copyright (C) 2014, Siemens AG
 * Author: Daniele Fognini, Johannes Najjar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "copyrightUtils.hpp"
#include <boost/program_options.hpp>

#include <iostream>

using namespace std;

void queryAgentId(int& agent, PGconn* dbConn)
{
  char* SVN_REV = fo_sysconfig(AGENT_NAME, "SVN_REV");
  char* VERSION = fo_sysconfig(AGENT_NAME, "VERSION");
  char* agentRevision;
  if (!asprintf(&agentRevision, "%s.%s", VERSION, SVN_REV))
  {
    exit(-1);
  };

  int agentId = fo_GetAgentKey(dbConn,
    AGENT_NAME, 0, agentRevision, AGENT_DESC);
  free(agentRevision);

  if (agentId > 0)
  {
    agent = agentId;
  }
  else
  {
    exit(1);
  }
}

int writeARS(CopyrightState& state, int arsId, int uploadId, int success, const fo::DbManager& dbManager)
{
  return fo_WriteARS(dbManager.getConnection(), arsId, uploadId, state.getAgentId(), AGENT_ARS, NULL, success);
}

void bail(int exitval)
{
  fo_scheduler_disconnect(exitval);
  exit(exitval);
}

bool parseCliOptions(int argc, char** argv, CliOptions& dest, std::vector<std::string>& fileNames)
{
  unsigned type;

  boost::program_options::options_description desc(IDENTITY ": recognized options");
  desc.add_options()
        ("help,h", "shows help")
        (
          "type,T",
          boost::program_options::value<unsigned>(&type)
            ->default_value(ALL_TYPES),
          "type of regex to try"
        ) // TODO change and add help based on IDENTITY
        (
          "verbose,v", "increase verbosity"
        )
        (
          "regex",
          boost::program_options::value<vector<string> >(),
          "user defined Regex to search: [{name=cli}@@][{matchingGroup=0}@@]{regex} e.g. 'linux@@1@@(linus) torvalds'"
        )
        (
          "files",
          boost::program_options::value< vector<string> >(),
          "files to scan"
        );

  boost::program_options::positional_options_description p;
  p.add("files", -1);

  boost::program_options::variables_map vm;

  try
  {
    boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    type = vm["type"].as<unsigned>();

    if ((vm.count("help") > 0) || (type > ALL_TYPES))
    {
      cout << desc << endl;
      return false;
    }

    if (vm.count("files"))
    {
      fileNames = vm["files"].as<std::vector<string> >();
    }

    unsigned long verbosity = vm.count("verbose");

    dest = CliOptions(verbosity, type);

    if (vm.count("regex"))
    {
      const std::vector<std::string>& userRegexesFmts = vm["regex"].as<vector<std::string> >();
      for (auto it = userRegexesFmts.begin(); it != userRegexesFmts.end(); ++it) {
        scanner* sc = makeRegexScanner(*it, "cli");
        if (!(sc))
        {
          cout << "cannot parse regex format : " << *it << endl;
          return false;
        }
        else
        {
          dest.addScanner(sc);
        }
      }
    }

    return true;
  }
  catch (boost::bad_any_cast&) {
    cout << "wrong parameter type" << endl;
    cout << desc << endl;
    return false;
  }
  catch (boost::program_options::error&)
  {
    cout << "wrong command line arguments" << endl;
    cout << desc << endl;
    return false;
  }
}

static void addDefaultScanners(CopyrightState& state)
{
  unsigned types = state.getCliOptions().getOptType();
#ifdef IDENTITY_COPYRIGHT
  if (types & 1<<0)
    //state.addMatcher(RegexMatcher(regCopyright::getType(), regCopyright::getRegex()));
    state.addScanner(new hCopyrightScanner());

  if (types & 1<<1)
    state.addScanner(new regexScanner(regURL::getRegex(), regURL::getType()));

  if (types & 1<<2)
    state.addScanner(new regexScanner(regEmail::getRegex(), regEmail::getType(), 1));

  if (types & 1<<3)
    state.addScanner(new regexScanner(regAuthor::getRegex(), regAuthor::getType()));
#endif

#ifdef IDENTITY_ECC
  if (types & 1<<0)
    state.addScanner(new regexScanner(regEcc::getRegex(), regEcc::getType()));
#endif
}

static std::string getRegexConfFile(const std::string& identity)
{
  return std::string(sysconfigdir) + "/mods-enabled/" + identity +  "/agent/scanners.conf";
}

static void addConfFileScanners(CopyrightState& state)
{
  std::string confFile = getRegexConfFile(IDENTITY);
  ifstream stream(confFile);
  if (stream)
  {
    for (std::string line; std::getline(stream, line); )
    {
      scanner* sc = makeRegexScanner(line, IDENTITY);
      if (sc)
      {
        if (state.getCliOptions().isVerbosityDebug())
          cout << "loaded scanner definition: " << line << endl;
        state.addScanner(sc);
      }
      else
      {
        cout << "bad scanner definition in conf: " << line << endl;
      }
    }
  } else {
    if (state.getCliOptions().isVerbosityDebug())
      cout << "cannot open scanner definition in conf: " << confFile << endl;
  }
}

scanner* makeRegexScanner(const std::string& regexDesc, const std::string& defaultType) {
  #define RGX_FMT_SEPARATOR "@@"
  auto fmtRegex = rx::regex(
    "(?:([[:alpha:]]+)" RGX_FMT_SEPARATOR ")?(?:([[:digit:]]+)" RGX_FMT_SEPARATOR ")?(.*)",
    rx::regex_constants::icase
  );

  rx::match_results<std::string::const_iterator> match;
  if (rx::regex_match(regexDesc.begin(), regexDesc.end(), match, fmtRegex))
  {
    std::string type(match.length(1) > 0 ? match.str(1) : defaultType.c_str());

    int regId = match.length(2) > 0 ? std::stoi(std::string(match.str(2))) : 0;

    if (match.length(3) == 0)
      return 0; // nullptr

    std::string regexPattern(match.str(3));

    return new regexScanner(regexPattern, type, regId);
  }
  return 0; // nullptr
}

CopyrightState getState(fo::DbManager dbManager, CliOptions&& cliOptions)
{
  int agentID;
  queryAgentId(agentID, dbManager.getConnection());

  CopyrightState state(agentID, std::move(cliOptions));

  addDefaultScanners(state);
  addConfFileScanners(state);

  return state;
}

bool saveToDatabase(const string& s, const list<match>& matches, unsigned long pFileId, int agentId, const CopyrightDatabaseHandler& copyrightDatabaseHandler)
{
  if (!copyrightDatabaseHandler.begin())
  {
    return false;
  }

  size_t count = 0;
  for (auto m = matches.begin(); m != matches.end(); ++m)
  {

    DatabaseEntry entry;
    entry.agent_fk = agentId;
    entry.content = cleanMatch(s, *m);
    entry.copy_endbyte = m->end;
    entry.copy_startbyte = m->start;
    entry.pfile_fk = pFileId;
    entry.type = m->type;

    if (entry.content.length() != 0)
    {
      ++count;
      if (!copyrightDatabaseHandler.insertInDatabase(entry))
      {
        copyrightDatabaseHandler.rollback();
        return false;
      };
    }
  }

  if (count == 0)
  {
    copyrightDatabaseHandler.insertNoResultInDatabase(agentId, pFileId);
  }

  return copyrightDatabaseHandler.commit();
}

void matchFileWithLicenses(const string& sContent, unsigned long pFileId, CopyrightState const& state, CopyrightDatabaseHandler& databaseHandler)
{
  list<match> l;
  const list<unptr::shared_ptr<scanner>>& scanners = state.getScanners();
  for (auto sc = scanners.begin(); sc != scanners.end(); ++sc)
  {
    (*sc)->ScanString(sContent, l);
  }
  saveToDatabase(sContent, l, pFileId, state.getAgentId(), databaseHandler);
}

void matchPFileWithLicenses(CopyrightState const& state, unsigned long pFileId, CopyrightDatabaseHandler& databaseHandler)
{
  char* pFile = databaseHandler.getPFileNameForFileId(pFileId);

  if (!pFile)
  {
    cout << "File not found " << pFileId << endl;
    bail(8);
  }

  char* fileName = NULL;
  {
#pragma omp critical (repo_mk_path)
    fileName = fo_RepMkPath("files", pFile);
  }
  if (fileName)
  {
    string s;
    ReadFileToString(fileName, s);

    matchFileWithLicenses(s, pFileId, state, databaseHandler);

    free(fileName);
    free(pFile);
  }
  else
  {
    cout << "PFile not found in repo " << pFileId << endl;
    bail(7);
  }
}

bool processUploadId(const CopyrightState& state, int uploadId, CopyrightDatabaseHandler& databaseHandler)
{
  vector<unsigned long> fileIds = databaseHandler.queryFileIdsForUpload(state.getAgentId(), uploadId);

#pragma omp parallel
  {
    CopyrightDatabaseHandler threadLocalDatabaseHandler(std::move(databaseHandler.spawn()));

    size_t pFileCount = fileIds.size();
#pragma omp for
    for (size_t it = 0; it < pFileCount; ++it)
    {
      unsigned long pFileId = fileIds[it];

      if (pFileId == 0)
      {
        continue;
      }

      matchPFileWithLicenses(state, pFileId, threadLocalDatabaseHandler);

      fo_scheduler_heart(1);
    }
  }

  return true;
}

