/* $Id$ */
/** @file
 * VBox Qt GUI - UIGuestOSType class implementation.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* GUI includes: */
#include "UIGuestOSType.h"

/* COM includes: */
#include "CGuestOSType.h"

/*********************************************************************************************************************************
*   UIGuestOSType definition.                                                                                     *
*********************************************************************************************************************************/

/** A wrapper around CGuestOSType. Some of the properties are cached here for performance. */
class SHARED_LIBRARY_STUFF UIGuestOSType
{

public:


    UIGuestOSType(const CGuestOSType &comGuestOSType);
    UIGuestOSType();

    const QString &getFamilyId() const;
    const QString &getFamilyDescription() const;
    const QString &getId() const;
    const QString &getVariant() const;
    const QString &getDescription() const;

    /** @name Wrapper getters for CGuestOSType member.
      * @{ */
        KStorageBus             getRecommendedHDStorageBus() const;
        ULONG                   getRecommendedRAM() const;
        KStorageBus             getRecommendedDVDStorageBus() const;
        ULONG                   getRecommendedCPUCount() const;
        KFirmwareType           getRecommendedFirmware() const;
        bool                    getRecommendedFloppy() const;
        LONG64                  getRecommendedHDD() const;
        KGraphicsControllerType getRecommendedGraphicsController() const;
        KStorageControllerType  getRecommendedDVDStorageController() const;
    /** @} */

    bool isOk() const;

private:

    /** @name CGuestOSType properties. Cached here for a faster access.
      * @{ */
        mutable QString m_strFamilyId;
        mutable QString m_strFamilyDescription;
        mutable QString m_strId;
        mutable QString m_strVariant;
        mutable QString m_strDescription;
    /** @} */

    CGuestOSType m_comGuestOSType;
};

/*********************************************************************************************************************************
*   UIGuestOSTypeManager implementaion.                                                                                     *
*********************************************************************************************************************************/

UIGuestOSTypeManager::UIGuestOSTypeManager()
    :m_guestOSTypes(new QList<UIGuestOSType>())
{
}

void UIGuestOSTypeManager::reCacheGuestOSTypes(const CGuestOSTypeVector &guestOSTypes)
{
    AssertReturnVoid(m_guestOSTypes);
    m_typeIdIndexMap.clear();
    m_guestOSTypes->clear();
    m_guestOSFamilies.clear();

    QVector<CGuestOSType> otherOSTypes;
    foreach (const CGuestOSType &comType, guestOSTypes)
    {
        if (comType.GetFamilyId().contains("other", Qt::CaseInsensitive))
        {
            otherOSTypes << comType;
            continue;
        }
        addGuestOSType(comType);
    }
    /* Add OS types with family other to the end of the lists: */
    foreach (const CGuestOSType &comType, otherOSTypes)
        addGuestOSType(comType);
}

void UIGuestOSTypeManager::addGuestOSType(const CGuestOSType &comType)
{
    AssertReturnVoid(m_guestOSTypes);
    m_guestOSTypes->append(UIGuestOSType(comType));
    m_typeIdIndexMap[m_guestOSTypes->last().getId()] = m_guestOSTypes->size() - 1;
    QPair<QString, QString> family = QPair<QString, QString>(m_guestOSTypes->last().getFamilyId(), m_guestOSTypes->last().getFamilyDescription());
    if (!m_guestOSFamilies.contains(family))
        m_guestOSFamilies << family;
}

const UIGuestOSTypeManager::UIGuestOSTypeFamilyInfo &UIGuestOSTypeManager::getFamilies() const
{
    return m_guestOSFamilies;
}

QStringList UIGuestOSTypeManager::getVariantListForFamilyId(const QString &strFamilyId) const
{
    AssertReturn(m_guestOSTypes, QStringList());
    QStringList variantList;
    foreach (const UIGuestOSType &type, *m_guestOSTypes)
    {
        if (type.getFamilyId() != strFamilyId)
            continue;
        const QString &strVariant = type.getVariant();
        if (!strVariant.isEmpty() && !variantList.contains(strVariant))
            variantList << strVariant;
    }
    return variantList;
}

UIGuestOSTypeManager::UIGuestOSTypeInfo UIGuestOSTypeManager::getTypeListForFamilyId(const QString &strFamilyId) const
{
    UIGuestOSTypeInfo typeInfoList;
    AssertReturn(m_guestOSTypes, typeInfoList);
    foreach (const UIGuestOSType &type, *m_guestOSTypes)
    {
        if (type.getFamilyId() != strFamilyId)
            continue;
        QPair<QString, QString> info(type.getId(), type.getDescription());

        if (!typeInfoList.contains(info))
            typeInfoList << info;
    }
    return typeInfoList;
}

UIGuestOSTypeManager::UIGuestOSTypeInfo UIGuestOSTypeManager::getTypeListForVariant(const QString &strVariant) const
{
    UIGuestOSTypeInfo typeInfoList;
    AssertReturn(m_guestOSTypes, typeInfoList);
    if (strVariant.isEmpty())
        return typeInfoList;

    foreach (const UIGuestOSType &type, *m_guestOSTypes)
    {
        if (type.getVariant() != strVariant)
            continue;
        QPair<QString, QString> info(type.getId(), type.getDescription());
        if (!typeInfoList.contains(info))
            typeInfoList << info;
    }
    return typeInfoList;
}

QString UIGuestOSTypeManager::getFamilyId(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, QString());
    /* Let QVector<>::value check for the bounds. It returns a default constructed value when it is out of bounds. */
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getFamilyId();
}

QString UIGuestOSTypeManager::getVariant(const QString  &strTypeId) const
{
    AssertReturn(m_guestOSTypes, QString());
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getVariant();
}

KGraphicsControllerType UIGuestOSTypeManager::getRecommendedGraphicsController(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, KGraphicsControllerType_Null);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedGraphicsController();
}

KStorageControllerType UIGuestOSTypeManager::getRecommendedDVDStorageController(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, KStorageControllerType_Null);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedDVDStorageController();
}

ULONG UIGuestOSTypeManager::getRecommendedRAM(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, 0);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedRAM();
}

ULONG UIGuestOSTypeManager::getRecommendedCPUCount(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, 0);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedCPUCount();
}

KFirmwareType UIGuestOSTypeManager::getRecommendedFirmware(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, KFirmwareType_Max);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedFirmware();
}

QString UIGuestOSTypeManager::getDescription(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, QString());
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getDescription();
}

LONG64 UIGuestOSTypeManager::getRecommendedHDD(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, 0);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedHDD();
}

KStorageBus UIGuestOSTypeManager::getRecommendedHDStorageBus(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, KStorageBus_Null);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedHDStorageBus();
}

KStorageBus UIGuestOSTypeManager::getRecommendedDVDStorageBus(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, KStorageBus_Null);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedDVDStorageBus();
}

bool UIGuestOSTypeManager::getRecommendedFloppy(const QString &strTypeId) const
{
    AssertReturn(m_guestOSTypes, false);
    return m_guestOSTypes->value(m_typeIdIndexMap.value(strTypeId, -1)).getRecommendedFloppy();
}

bool UIGuestOSTypeManager::isLinux(const QString &strTypeId) const
{
    QString strFamilyId = getFamilyId(strTypeId);
    if (strFamilyId.contains("linux", Qt::CaseInsensitive))
        return true;
    return false;
}

bool UIGuestOSTypeManager::isWindows(const QString &strTypeId) const
{
    QString strFamilyId = getFamilyId(strTypeId);
    if (strFamilyId.contains("windows", Qt::CaseInsensitive))
        return true;
    return false;
}

/* static */
bool UIGuestOSTypeManager::isDOSType(const QString &strOSTypeId)
{
    if (   strOSTypeId.left(3) == "dos"
        || strOSTypeId.left(3) == "win"
        || strOSTypeId.left(3) == "os2")
        return true;

    return false;
}

/*********************************************************************************************************************************
*   UIGuestOSType implementaion.                                                                                     *
*********************************************************************************************************************************/

UIGuestOSType::UIGuestOSType()
{
}

UIGuestOSType::UIGuestOSType(const CGuestOSType &comGuestOSType)
    : m_comGuestOSType(comGuestOSType)
{
}

bool UIGuestOSType::isOk() const
{
    return (!m_comGuestOSType.isNull() && m_comGuestOSType.isOk());
}

const QString &UIGuestOSType::getFamilyId() const
{
    if (m_strFamilyId.isEmpty() && m_comGuestOSType.isOk())
        m_strFamilyId = m_comGuestOSType.GetFamilyId();
    return m_strFamilyId;
}

const QString &UIGuestOSType::getFamilyDescription() const
{
    if (m_strFamilyDescription.isEmpty() && m_comGuestOSType.isOk())
        m_strFamilyDescription = m_comGuestOSType.GetFamilyDescription();
    return m_strFamilyDescription;
}

const QString &UIGuestOSType::getId() const
{
    if (m_strId.isEmpty() && m_comGuestOSType.isOk())
        m_strId = m_comGuestOSType.GetId();
    return m_strId;
}

const QString &UIGuestOSType::getVariant() const
{
    if (m_strVariant.isEmpty() && m_comGuestOSType.isOk())
        m_strVariant = m_comGuestOSType.GetVariant();
    return m_strVariant;
}

const QString &UIGuestOSType::getDescription() const
{
    if (m_strDescription.isEmpty() && m_comGuestOSType.isOk())
        m_strDescription = m_comGuestOSType.GetDescription();
    return m_strDescription;
}

KStorageBus UIGuestOSType::getRecommendedHDStorageBus() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedHDStorageBus();
    return KStorageBus_Null;
}

ULONG UIGuestOSType::getRecommendedRAM() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedRAM();
    return 0;
}

KStorageBus UIGuestOSType::getRecommendedDVDStorageBus() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedDVDStorageBus();
    return KStorageBus_Null;
}

ULONG UIGuestOSType::getRecommendedCPUCount() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedCPUCount();
    return 0;
}

KFirmwareType UIGuestOSType::getRecommendedFirmware() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedFirmware();
    return  KFirmwareType_Max;
}

bool UIGuestOSType::getRecommendedFloppy() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedFloppy();
    return false;
}

LONG64 UIGuestOSType::getRecommendedHDD() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedHDD();
    return 0;
}

KGraphicsControllerType UIGuestOSType::getRecommendedGraphicsController() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedGraphicsController();
    return KGraphicsControllerType_Null;
}

KStorageControllerType UIGuestOSType::getRecommendedDVDStorageController() const
{
    if (m_comGuestOSType.isOk())
        return m_comGuestOSType.GetRecommendedDVDStorageController();
    return KStorageControllerType_Null;
}
