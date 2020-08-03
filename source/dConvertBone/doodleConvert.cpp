#include <maya/MGlobal.h>
#include "doodleConvert.h"
#include <maya/MString.h>
DoodleDemBones::DoodleDemBones( )
{
}

DoodleDemBones::~DoodleDemBones( )
{
}

void DoodleDemBones::cbInitSplitBegin( )
{
    MGlobal::displayInfo("��ʼ�ָ�");
}

void DoodleDemBones::cbInitSplitEnd( )
{
    //MGlobal::displayInfo("�ָ�������� : " + nB);
}

void DoodleDemBones::cbIterBegin( )
{
    //MGlobal::displayInfo("�������� # " + iter);
}

bool DoodleDemBones::cbIterEnd( )
{
    double rmseValue = rmse( );
    MGlobal::displayInfo("����ֵ = " + MString() + rmseValue);
    if (this->dolCom.isInterruptRequested( )) {
        return true;
    }
    return false;
}

void DoodleDemBones::cbWeightsBegin( )
{
    //MGlobal::displayInfo("����Ȩ��");
}

void DoodleDemBones::cbWeightsEnd( )
{
    //MGlobal::displayInfo("����Ȩ�ؽ���");
}

void DoodleDemBones::cbTranformationsBegin( )
{
    //MGlobal::displayInfo("����ת��");
}

void DoodleDemBones::cbTransformationsEnd( )
{
    //MGlobal::displayInfo("����ת������");
}

void DoodleDemBones::cbWeightsIterBegin( )
{
    //MGlobal::displayInfo("Ȩ�ص�����ʼ");
}

bool DoodleDemBones::cbWeightsIterEnd( )
{
    MGlobal::displayInfo("Ȩ�ص�������");
    if (this->dolCom.isInterruptRequested( )) {
        return true;
    }
    return false;
}
