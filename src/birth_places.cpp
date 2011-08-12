// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include <algorithm>
#include <sstream>

#include "conf.h"
#include "sysdep.h"
#include "utils.h"
#include "comm.h"
//#include "db.h"
//#include "dg_scripts.h"
//#include "char.hpp"
//#include "handler.h"

#include "birth_places.hpp"
#include "pugixml.hpp"

BirthPlaceListType BirthPlace::BirthPlaceList;

//������� ����� ���� � ��������� �� ���� ���������� �� ������
void BirthPlace::LoadBirthPlace(pugi::xml_node BirthPlaceNode)
{
	pugi::xml_node CurNode;
	BirthPlacePtr TmpBirthPlace(new BirthPlace);

	//������ ��������� ����� ��������
    TmpBirthPlace->_Id = BirthPlaceNode.attribute("id").as_int();
	TmpBirthPlace->_Name = BirthPlaceNode.child("name").child_value();
    TmpBirthPlace->_Description = BirthPlaceNode.child("shortdesc").child_value();
    TmpBirthPlace->_MenuStr = BirthPlaceNode.child("menustring").child_value();
    CurNode = BirthPlaceNode.child("room");
    TmpBirthPlace->_LoadRoom = CurNode.attribute("vnum").as_int();

	//������ ������ ���������
	CurNode = BirthPlaceNode.child("items");
	for (CurNode = CurNode.child("item"); CurNode; CurNode = CurNode.next_sibling("item"))
	{
		TmpBirthPlace->_ItemsList.push_back(CurNode.attribute("vnum").as_int());
	}
	//��������� ����� ����� � ������
	BirthPlace::BirthPlaceList.push_back(TmpBirthPlace);
}

//�������� ���������� ����� �������� ����������
void BirthPlace::Load(const char *PathToFile)
{
	char buf[MAX_INPUT_LENGTH];
	pugi::xml_document Doc;
	pugi::xml_node BirthPlaceList, BirthPlaceNode;
	pugi::xml_parse_result Result;

	Result = Doc.load_file(PathToFile);
	if (!Result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", Result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	BirthPlaceList = Doc.child("birthplaces");
	if (!BirthPlaceList)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...birth places reading fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

    // ������ ����� ����� ����� ����������
	for (BirthPlaceNode = BirthPlaceList.child("birthplace"); BirthPlaceNode; BirthPlaceNode = BirthPlaceNode.next_sibling("birthplace"))
	{
		BirthPlace::LoadBirthPlace(BirthPlaceNode);
	}
}

// ���� ���� map ������������. %) ������ ���������
// ���� ���� ������ - ����� ���������.

BirthPlacePtr BirthPlace::GetBirthPlaceById(short Id)
{
    BirthPlacePtr BPPtr;
    for (BirthPlaceListType::iterator it =  BirthPlaceList.begin();it != BirthPlaceList.end();++it)
        if (Id == (*it)->Id())
            BPPtr = *it;

    return BPPtr;
};

int BirthPlace::GetLoadRoom(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL)
        return BPPtr->LoadRoom();

    return DEFAULT_LOADROOM;
};

std::vector<int> BirthPlace::GetItemList(short Id)
{
    std::vector<int> BirthPlaceItemList;
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL)
        BirthPlaceItemList = BPPtr->ItemsList();

    return BirthPlaceItemList;
};

std::string BirthPlace::GetMenuStr(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL)
        return BPPtr->MenuStr();

    return BIRTH_PLACE_NAME_UNDEFINED;
};

std::string BirthPlace::ShowMenu(std::vector<int> BPList)
{
    int i;
    BirthPlacePtr BPPtr;
    std::ostringstream buffer;
    i = 1;
    for (std::vector<int>::iterator it = BPList.begin();it != BPList.end();++it)
    {
        BPPtr = BirthPlace::GetBirthPlaceById(*it);
        buffer << " " << i << ") " << BPPtr->_MenuStr << "\r\n";
        i++;
    }

     return buffer.str();
};

short BirthPlace::ParseSelect(char *arg)
{
    std::string select = arg;
    lower_convert(select);
//    std::transform(select.begin(), select.end(), select.begin(), _tolower);
    for (BirthPlaceListType::iterator it = BirthPlaceList.begin();it != BirthPlaceList.end();++it)
        if (select == (*it)->Description())
            return (*it)->Id();

    return BIRTH_PLACE_UNDEFINED;
};

bool BirthPlace::CheckId(short Id)
{
    BirthPlacePtr BPPtr = BirthPlace::GetBirthPlaceById(Id);
    if (BPPtr != NULL)
        return true;

    return false;
};

