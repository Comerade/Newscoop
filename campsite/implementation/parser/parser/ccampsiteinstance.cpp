/******************************************************************************

CAMPSITE is a Unicode-enabled multilingual web content
management system for news publications.
CAMPFIRE is a Unicode-enabled java-based near WYSIWYG text editor.
Copyright (C)2000,2001  Media Development Loan Fund
contact: contact@campware.org - http://www.campware.org
Campware encourages further development. Please let us know.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

******************************************************************************/

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <unistd.h>

#include "ccampsiteinstance.h"

using std::cout;
using std::endl;


/**
 * class CCampsiteInstance implementation
 *
 */

bool CCampsiteInstance::isRunning() const
{
	return m_bRunning;
}

pid_t CCampsiteInstance::run() throw (RunException)
{
	return -1;
}

void CCampsiteInstance::stop()
{
	if (!m_bRunning)
		return;
}

const CCampsiteInstanceMap& CCampsiteInstance::readFromDirectory(const string& p_rcoDir,
		InstanceFunction p_pInstFunc) throw (ConfException)
{
	DIR* pDir = opendir(p_rcoDir.c_str());
	if (pDir == NULL)
		throw ConfException(string("Invalid configuration directory ") + p_rcoDir);

	for (struct dirent* pFile = readdir(pDir); pFile != NULL; pFile = readdir(pDir))
	{
		if (strcmp(pFile->d_name, ".") == 0 || strcmp(pFile->d_name, "..") == 0)
			continue;

		string coFileName = p_rcoDir + "/" + pFile->d_name;
		struct stat FileStat;
		if (stat(coFileName.c_str(), &FileStat) != 0)
			continue;
		if (!S_ISDIR(FileStat.st_mode))
			continue;

		new CCampsiteInstance(coFileName, p_pInstFunc);
	}

	closedir(pDir);

	return CCampsiteInstanceRegister::get().getCampsiteInstances();
}

const ConfAttrValue& CCampsiteInstance::ReadConf() throw (ConfException)
{
	VerifyDir(m_coConfDir);

	string::size_type nSlashPos = m_coConfDir.rfind('/');
	while ((nSlashPos + 1) == m_coConfDir.length() && nSlashPos != string::npos)
	{
		m_coConfDir.erase(nSlashPos, 1);
		nSlashPos = m_coConfDir.rfind('/');
	}
	m_coName = m_coConfDir.substr(nSlashPos != string::npos ? nSlashPos + 1 : 0);

	// read parser configuration
	string coParserConfFile = m_coConfDir + "/parser_conf.php";
	m_coAttributes.open(coParserConfFile);

	// read apache configuration
	string coApacheConfFile = m_coConfDir + "/apache_conf.php";
	m_coAttributes.open(coApacheConfFile);
	struct passwd* pPwEnt = getpwnam(m_coAttributes.valueOf("APACHE_USER").c_str());
	if (pPwEnt == NULL)
		throw ConfException("Invalid user name in conf file");
	struct group* pGrEnt = getgrnam(m_coAttributes.valueOf("APACHE_GROUP").c_str());
	if (pGrEnt == NULL)
		throw ConfException("Invalid group name in conf file");

	// read database configuration
	string coDatabaseConfFile = m_coConfDir + "/database_conf.php";
	m_coAttributes.open(coDatabaseConfFile);

	return m_coAttributes;
}

void CCampsiteInstance::RegisterInstance()
{
	CCampsiteInstanceRegister::get().insert(*this);
}

void CCampsiteInstance::VerifyDir(const string& p_rcoDir) throw (ConfException)
{
	DIR* pDir = opendir(p_rcoDir.c_str());
	if (pDir == NULL)
		throw ConfException(string("Invalid configuration directory ") + p_rcoDir);
	closedir(pDir);
}


/**
 * class CCampsiteInstanceRegister implementation
 *
 */

CCampsiteInstanceRegister g_coCampsiteInstanceRegister;


CCampsiteInstanceRegister& CCampsiteInstanceRegister::get()
{
	return g_coCampsiteInstanceRegister;
}

void CCampsiteInstanceRegister::erase(pid_t p_nInstancePID)
{
#ifdef _REENTRANT
	CMutexHandler coLockHandler(&m_coMutex);
#endif
	map < pid_t, string, less<pid_t> >::iterator coIt;
	coIt = m_coInstancePIDs.find(p_nInstancePID);
	if (coIt != m_coInstancePIDs.end())
		erase((*coIt).second);
}

void CCampsiteInstanceRegister::erase(const string& p_rcoInstanceName)
{
#ifdef _REENTRANT
	CMutexHandler coLockHandler(&m_coMutex);
#endif
	CCampsiteInstanceMap::const_iterator coIt;
	coIt = m_coCCampsiteInstances.find(p_rcoInstanceName);
	if (coIt == m_coCCampsiteInstances.end())
		return;
	CCampsiteInstance* pcoInstance = (*coIt).second;
	if (pcoInstance->isRunning())
		m_coInstancePIDs.erase(pcoInstance->getPID());
	m_coCCampsiteInstances.erase(p_rcoInstanceName);
	delete pcoInstance;
}

const CCampsiteInstance* CCampsiteInstanceRegister::getCampsiteInstance(pid_t p_nInstancePID) const
		throw (InvalidValue)
{
#ifdef _REENTRANT
	CMutexHandler coLockHandler(&m_coMutex);
#endif
	map < pid_t, string, less<pid_t> >::const_iterator coIt;
	coIt = m_coInstancePIDs.find(p_nInstancePID);
	if (coIt == m_coInstancePIDs.end())
		throw InvalidValue("Campsite instance name");
	return getCampsiteInstance((*coIt).second);
}

const CCampsiteInstance* CCampsiteInstanceRegister::getCampsiteInstance(const string& p_rcoInstanceName) const
		throw (InvalidValue)
{
#ifdef _REENTRANT
	CMutexHandler coLockHandler(&m_coMutex);
#endif
	CCampsiteInstanceMap::const_iterator coIt;
	coIt = m_coCCampsiteInstances.find(p_rcoInstanceName);
	if (coIt == m_coCCampsiteInstances.end())
		throw InvalidValue("Campsite instance PID");
	return (*coIt).second;
}
