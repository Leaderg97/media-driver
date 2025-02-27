/*
* Copyright (c) 2021-2022, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file     media_user_setting_configure.cpp
//! \brief    The interface of media user setting configure.

#include "mos_os.h"
#include "media_user_setting_configure.h"

namespace MediaUserSetting {
namespace Internal {

const UFKEY_NEXT Configure::m_rootKey = UFKEY_INTERNAL_NEXT;
const char *Configure::m_configPath = USER_SETTING_CONFIG_PATH;
const char *Configure::m_reportPath = USER_SETTING_REPORT_PATH;

Configure::Configure(MOS_USER_FEATURE_KEY_PATH_INFO *keyPathInfo):Configure()
{
    m_keyPathInfo = keyPathInfo;

    std::string subPath = "";

    if (m_keyPathInfo != nullptr && m_keyPathInfo->Path != nullptr)
    {
        subPath = m_keyPathInfo->Path;
    }

    //when statePath set, will init m_statedConfigPath and m_statedReportPath with m_keyPathInfo
    m_statedConfigPath = subPath + m_configPath;
    m_statedReportPath = subPath + m_reportPath;
}

Configure::Configure()
{
#if (_DEBUG || _RELEASE_INTERNAL)
        m_isDebugMode = true;
#endif
    m_statedConfigPath = m_configPath;
    m_statedReportPath = m_reportPath;

    MosUtilities::MosInitializeReg(m_regBufferMap);
}

Configure::~Configure()
{
    MosUtilities::MosUninitializeReg(m_regBufferMap);
}

MOS_STATUS Configure::Register(
    const std::string &valueName,
    const Group &group,
    const Value &defaultValue,
    bool isReportKey,
    bool debugOnly,
    bool useCustomPath,
    const std::string &customPath,
    bool statePath)
{
    m_mutexLock.Lock();

    if (IsDefinitionExist(valueName))
    {
        m_mutexLock.Unlock();
        return MOS_STATUS_FILE_EXISTS;
    }

    auto &defs = GetDefinitions(group);
    std::string subPath = "";
    if (useCustomPath)
    {
        if (statePath && m_keyPathInfo != nullptr && m_keyPathInfo->Path != nullptr)
        {
            subPath = m_keyPathInfo->Path;
        }
        subPath += customPath;
    }
    else
    {
        if (statePath)
        {
            subPath = m_statedConfigPath;
        }
        else
        {
            subPath = m_configPath;
        }
    }

    defs.insert(
        std::make_pair(
            MakeHash(valueName),
            std::make_shared<Definition>(
                valueName,
                defaultValue,
                isReportKey,
                debugOnly,
                useCustomPath,
                subPath,
                m_rootKey,
                statePath)));

    m_mutexLock.Unlock();

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Configure::Read(Value &value,
    const std::string &valueName,
    const Group &group,
    const Value &customValue,
    bool useCustomValue,
    uint32_t option)
{
    int32_t ret = 0;

    auto &defs = GetDefinitions(group);

    auto def = defs[MakeHash(valueName)];
    if (def == nullptr)
    {
        return MOS_STATUS_INVALID_HANDLE;
    }

    if (def->IsDebugOnly() && !m_isDebugMode)
    {
        value = useCustomValue ? customValue : def->DefaultValue();
        return MOS_STATUS_SUCCESS;
    }

    std::string path = GetReadPath(def, option);

    UFKEY_NEXT  key      = {};
    std::string strValue = "";
    uint32_t    size     = MOS_USER_CONTROL_MAX_DATA_SIZE;
    uint32_t    type     = 0;

    // read env variable first, if env value is set, return
    // else read the reg keys
    MOS_STATUS status = MosUtilities::MosReadEnvVariable(key, valueName, &type, strValue, &size);
 
    if (status == MOS_STATUS_SUCCESS)
    {
        value = strValue;
        return MOS_STATUS_SUCCESS;
    }

    status = MosUtilities::MosOpenRegKey(m_rootKey, path, KEY_READ, &key, m_regBufferMap);
    
    if (status == MOS_STATUS_SUCCESS)
    {
        strValue = "";
        size     = MOS_USER_CONTROL_MAX_DATA_SIZE;
        type     = 0;

        m_mutexLock.Lock();
        status = MosUtilities::MosGetRegValue(key, valueName, &type, strValue, &size, m_regBufferMap);
        if (status == MOS_STATUS_SUCCESS)
        {
            value = strValue;
        }
        m_mutexLock.Unlock();

        MosUtilities::MosCloseRegKey(key);
    }

    if (status != MOS_STATUS_SUCCESS && option == MEDIA_USER_SETTING_INTERNAL)
    {
        value = useCustomValue ? customValue : def->DefaultValue();
    }

    return status;
}

MOS_STATUS Configure::Write(
    const std::string &valueName,
    const Value &value,
    const Group &group,
    bool isForReport,
    uint32_t option)
{
    auto &defs = GetDefinitions(group);

    auto def = defs[MakeHash(valueName)];
    if (def == nullptr)
    {
        return MOS_STATUS_INVALID_HANDLE;
    }

    if (def->IsDebugOnly() && !m_isDebugMode)
    {
        return MOS_STATUS_SUCCESS;
    }

    if (!def->IsReportKey() && isForReport)
    {
        return MOS_STATUS_INVALID_PARAMETER;
    }

    std::string path = GetReportPath(def, option);

    UFKEY_NEXT key = {};
    MOS_STATUS status = MOS_STATUS_UNKNOWN;

    m_mutexLock.Lock();
    status = MosUtilities::MosCreateRegKey(m_rootKey, path, KEY_WRITE, &key, m_regBufferMap);

    if (status == MOS_STATUS_SUCCESS)
    {
        uint32_t size = def->DefaultValue().Size();
        uint32_t value_type;
        switch (def->DefaultValue().ValueType())
        {
        case MOS_USER_FEATURE_VALUE_TYPE_BOOL:
            value_type = UF_BINARY;
            break;
        case MOS_USER_FEATURE_VALUE_TYPE_FLOAT:
        case MOS_USER_FEATURE_VALUE_TYPE_UINT32:
        case MOS_USER_FEATURE_VALUE_TYPE_INT32:
            value_type = UF_DWORD;
            break;
        case MOS_USER_FEATURE_VALUE_TYPE_UINT64:
        case MOS_USER_FEATURE_VALUE_TYPE_INT64:
            value_type = UF_QWORD;
            break;
        case MOS_USER_FEATURE_VALUE_TYPE_MULTI_STRING:
        case MOS_USER_FEATURE_VALUE_TYPE_STRING:
            value_type = UF_SZ;
            break;
        default:
            value_type = UF_NONE;
            break;
        }

        status = MosUtilities::MosSetRegValue(key, valueName, value_type, value.ConstString(), m_regBufferMap);

        MosUtilities::MosCloseRegKey(key);
    }
    m_mutexLock.Unlock();

    if (status != MOS_STATUS_SUCCESS)
    {
        // When any fail happen, just print out a critical message, but not return error to break normal call sequence.
        if(MosUtilities::m_mosUltFlag)
        {
            MOS_OS_NORMALMESSAGE("Failed to write media user setting %s value.", valueName.c_str());
        }
        else
        {
            MOS_OS_NORMALMESSAGE("Failed to write media user setting %s value.", valueName.c_str());
        }
    }

    return MOS_STATUS_SUCCESS;
}

std::string Configure::GetReadPath(
    std::shared_ptr<Definition> def,
    uint32_t option)
{
    std::string path = "";
    if (def == nullptr)
    {
        return path;
    }

    if (option == MEDIA_USER_SETTING_INTERNAL)
    {
        return def->GetSubPath();
    }
    else
    {
        return GetExternalPath(option);
    }
}

std::string Configure::GetReportPath(
    std::shared_ptr<Definition> def,
    uint32_t option)
{
    std::string path = "";
    if (def == nullptr)
    {
        return path;
    }
    if (option == MEDIA_USER_SETTING_INTERNAL)
    {
        return m_statedReportPath;
    }
    else
    {
        return GetExternalPath(option);
    }
}

std::string Configure::GetExternalPath(uint32_t option)
{
    std::string path = "";
    auto it = m_pathOption.find(option);
    if (it != m_pathOption.end())
    {
        if (it->second.bStated && m_keyPathInfo != nullptr && m_keyPathInfo->Path != nullptr)
        {
            path = m_keyPathInfo->Path;
        }
        path += it->second.subPath;
        return path;
    }
    else
    {
        MOS_OS_ASSERTMESSAGE("Invalid Option");
        return "";
    }
}

}}