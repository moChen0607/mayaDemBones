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
class DoodleConvertBone : public MPxNode
{
public:
	DoodleConvertBone();
	~DoodleConvertBone() override;
	static void* creator();

	MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

	static MStatus initialize( );

	void SetSubObjectIndex( );

	void initConvertAttr(MFnMesh& inputMesh_);

	void GetFrameMeshData(int i, MObject& MobjMesh);

	void GetBindFrame(MObject& MobjMesh);

	void GetMeshData();

	MStatus setOutBindWeight(MDataBlock& dataBlock);

	MStatus setBindPose(MDataBlock& dataBlock);

	MStatus setAim(MDataBlock& dataBlock);
	//void createJoins(const std::vector<MString>& name);

	//void addCurve();



	MStatus AnalysisCommand(MDataBlock & datablock );
	
	MStatus InitAndCimpute( );
	// ����ת����ʵ��
	DoodleDemBones DoodleConvert;
	
	std::vector<MFnIkJoint> doolJoint;
	
	MDGModifier dgModidier;
	// �Ƚ���Ҫ��ֵ
	static MObject startFrame;
	static MObject endFrame;
	static MObject inputMesh;
	static MObject bindFrame;
	static MObject nBones;

	// �������ֵ
	static MObject nInitIters;
	static MObject nIters;
	static MObject nTransIters;
	static MObject isBindUpdate;
	static MObject transAffine;
	static MObject transAffineNorm;
	static MObject nWeightsIters;
	static MObject nonZeroWeightsNum;
	static MObject weightsSmooth;
	static MObject weightsSmoothStep;

	// ��ƤȨ��
	static MObject bindWeights;
	static MObject bindWeightsList;
	// ����������
	//static MObject subjectIndex;
	// �������
	static MObject localAnim;
	static MObject localAnimList;
	// ȫ�ְ󶨾������
	static MObject globalBindMatrices;
	static MObject globalBindMatricesList;
	// ���ذ�pose
	static MObject localBindPose;
	static MObject localBindPoseList;

	// id
	static const MTypeId id;
private:

	int _startFrame_;
	int _endFrame_;
	int _bindFrame_;
	int _nBones_;
	int _nInitIters_;
	int _nIters_;
	int _nTransIters_;
	int _isBindUpdate_;
	double _transAffine_;
	double _transAffineNorm_;
	int _nWeightsIters_;
	int _nonZeroWeightsNum_;
	double _weightsSmooth_;
	double _weightsSmoothStep_;

	MObject _inputmesh_;


	// ��ƤȨ��
	Eigen::MatrixXd _bindWeights_;
	// �ο����
	Eigen::MatrixXd _localRotation_;
	// �ο�ת��
	Eigen::MatrixXd _localTranslation_;
	// ȫ�ְ󶨾������
	Eigen::MatrixXd _globalBindMatrices_;
	// ������ת��pose
	Eigen::MatrixXd _localBindPoseRotation_;
	// �������ƽ��pose
	Eigen::MatrixXd _localBindPoseTranslation_;

	int __bindFrame__;
	bool IsGetFrame;
};
