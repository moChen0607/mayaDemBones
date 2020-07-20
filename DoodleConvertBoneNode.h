//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

// MAYA HEADERS
#include <set>

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MSyntax.h>

class DoodleDemBones : public Dem::DemBonesExt<double, float> {
public:
	DoodleDemBones();
	~DoodleDemBones();
};


// MAIN CLASS 
class DoodleConvertBone : public MPxCommand
{
public:
	DoodleConvertBone();
	~DoodleConvertBone() override;
	static void* creator();

	static MStatus initialize();

	MStatus doIt(const MArgList& arge) override;

	static MSyntax createSyntax();
	//// ��ƤȨ��
	//static MObject bindWeights;
	//// ����������
	//static MObject subjectIndex;
	//// �ο����
	//static MObject aimRotation;
	//// �ο�ת��
	//static MObject aimTranslation;
	//// ȫ�ְ󶨾������
	//static MObject globalBindMatrices;
	//// ������ת��pose
	//static MObject localBindPoseRotation;
	//// �������ƽ��pose
	//static MObject localBindPoseTranslation;
private:
	DoodleDemBones DoodleConvert;
	int __bindFrame__;
	std::set<double> getFrame;
	bool IsGetFrame;
};
