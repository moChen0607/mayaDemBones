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

#include <math.h>

#include <maya/MIOStream.h>
#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MNurbsIntersector.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnNurbsSurface.h>

#include <maya/MArgDatabase.h>
#include <maya/MAnimControl.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MComputation.h>
#include <maya/MTime.h>
#include <maya/MFnMesh.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <Eigen/Dense>
#include <DemBones/DemBonesExt.h>

#include "DoodleConvertBoneNode.h"


// CONSTRUCTOR:
DoodleConvertBone::DoodleConvertBone()
{
    this->__bindFrame__ = 0;
    this->IsGetFrame = false;
    this->getFrame.clear();
}

// DESTRUCTOR:
DoodleConvertBone::~DoodleConvertBone()
{
}

// FOR CREATING AN INSTANCE OF THIS COMMAND:
void* DoodleConvertBone::creator()
{
    return new DoodleConvertBone;
}




MStatus DoodleConvertBone::initialize()
{
    MStatus state;
    return state;
}

MSyntax DoodleConvertBone::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-sf", "-startFrame", MSyntax::kDouble);
    syntax.addFlag("-ef", "-endFrame", MSyntax::kDouble);
    syntax.addFlag("-bf", "-bindFrame", MSyntax::kDouble);
    syntax.addFlag("-nb", "-nBones", MSyntax::kDouble);
    syntax.addFlag("-nin", "-nInitIters", MSyntax::kDouble);
    syntax.addFlag("-nit", "-nIters", MSyntax::kDouble);
    syntax.addFlag("-ntr", "-nTransIters", MSyntax::kDouble);
    syntax.addFlag("-bup", "-isBindUpdate", MSyntax::kBoolean);
    syntax.addFlag("-tra", "-transAffine", MSyntax::kDouble);
    syntax.addFlag("-tan", "-transAffineNorm", MSyntax::kDouble);
    syntax.addFlag("-nwi", "-nWeightsIters", MSyntax::kDouble);
    syntax.addFlag("-non", "-nonZeroWeightsNum", MSyntax::kDouble);
    syntax.addFlag("-ws", "-weightsSmooth", MSyntax::kDouble);
    syntax.addFlag("-wss", "-weightsSmoothStep", MSyntax::kDouble);
    syntax.addFlag("-im", "-inputMesh", MSyntax::kString);
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    return syntax;
}



MStatus DoodleConvertBone::doIt(const MArgList& arge)
{
    bool debug = true;
    bool treeBased = true;
    MStatus status = MStatus::kSuccess;
    //�����ĵ���ʱ��д
    MArgDatabase argData(syntax(), arge, &status);
    if (argData.isFlagSet("help")) {
        MGlobal::displayInfo("����ʲô��û��");
        return MS::kSuccess;
    }

    // �������ֵ
    int startFrame;
    int endFrame;
    int nBones = 30;
    int nInitIters = 10;
    int nIters = 30;
    int nTransIters = 5;
    int isBindUpdate = 0;
    double transAffine = 10;
    double transAffineNorm = 4;
    int nWeightsIters = 3;
    int nonZeroWeightsNum = 8;
    double weightsSmooth = 0.0001;
    double weightsSmoothStep = 0.5;
    MString inputMesh;
    //���Կ�ʼ֡�ͽ���֡
    if (argData.isFlagSet("startFrame"))
    {
        argData.getFlagArgument("startFrame", 0, startFrame);
    }
    else
    {
        MGlobal::displayError("��ָ����ʼ֡");
        return MS::kFailure;
    }
    if (argData.isFlagSet("endFrame")) { argData.getFlagArgument("endFrame", 0, endFrame); }
    else
    {
        MGlobal::displayError("��ָ������֡");
        return MS::kFailure;
    }
    //���Թ�������
    if (argData.isFlagSet("nBones"))
    {
        argData.getFlagArgument("nBones", 0, nBones);
        if (nBones <= 0) {
            MGlobal::displayError("�����ĸ�������С����");
            return MS::kFailure;
        }
    }
    //�������ֵ
    if (argData.isFlagSet("nInitIters")) { argData.getFlagArgument("nInitIters", 0, nInitIters); }
    if (argData.isFlagSet("nIters")) { argData.getFlagArgument("nIters", 0, nIters); }
    if (argData.isFlagSet("nTransIters")) { argData.getFlagArgument("nTransIters", 0, nTransIters); }
    if (argData.isFlagSet("isBindUpdate")) { argData.getFlagArgument("isBindUpdate", 0, isBindUpdate); }
    if (argData.isFlagSet("transAffine")) { argData.getFlagArgument("transAffine", 0, transAffine); }
    if (argData.isFlagSet("transAffineNorm")) { argData.getFlagArgument("transAffineNorm", 0, transAffineNorm); }
    if (argData.isFlagSet("nWeightsIters")) { argData.getFlagArgument("nWeightsIters", 0, nWeightsIters); }
    if (argData.isFlagSet("weightsSmooth")) { argData.getFlagArgument("weightsSmooth", 0, weightsSmooth); }
    if (argData.isFlagSet("weightsSmoothStep")) { argData.getFlagArgument("weightsSmoothStep", 0, weightsSmoothStep); }
    if (argData.isFlagSet("inputMesh")) { argData.getFlagArgument("inputMesh", 0, inputMesh); }
    else
    {   //���������Ƿ����
        MGlobal::displayError("û��ָ����������");
        return MS::kFailure;
    }
    this->DoodleConvert.nB = nBones;
    this->DoodleConvert.nIters = nIters;
    this->DoodleConvert.nTransIters = nTransIters;
    this->DoodleConvert.nWeightsIters = nWeightsIters;
    this->DoodleConvert.bindUpdate = isBindUpdate;
    this->DoodleConvert.transAffine = transAffine;
    this->DoodleConvert.transAffineNorm = transAffineNorm;
    this->DoodleConvert.nnz = nonZeroWeightsNum;
    this->DoodleConvert.weightsSmooth = weightsSmooth;
    this->DoodleConvert.weightsSmoothStep = weightsSmoothStep;

    this->DoodleConvert.nS = 1;
    //������֡��
    this->DoodleConvert.nF = endFrame - startFrame + 1;

    //������ֻ��һ�������ת��
    MSelectionList sel;
    CHECK_MSTATUS(sel.add(inputMesh));

    MDagPath inputMeshPath;
    sel.getDagPath(0, inputMeshPath);
    if (inputMeshPath.apiType() != MFnData::Type::kMesh)
    { 
        MGlobal::displayError("������������"); 
        return MS::kFailure;
    }
    // ��dag·��ת����������
    CHECK_MSTATUS(inputMeshPath.extendToShape())
    //���������
    MFnMesh inputMesh_(inputMeshPath.node());
    //��ö���
    MPointArray aimPoint;
    //���þ�ֹ�Ķ�����
    this->DoodleConvert.nV = aimPoint.length();
    //����һЩȫ��ֵ
    this->DoodleConvert.v.resize(3 * this->DoodleConvert.nF, this->DoodleConvert.nV);
    this->DoodleConvert.fTime.resize(this->DoodleConvert.nF);
    this->DoodleConvert.fStart.resize(this->DoodleConvert.nS + 1);
    this->DoodleConvert.fStart(0) = startFrame;

    this->DoodleConvert.subjectID.resize(this->DoodleConvert.nF);
    for (int i = startFrame; i < (endFrame + 1 ); i++)
    {
        MAnimControl::setCurrentTime(MTime(i, MTime::uiUnit()));
        inputMesh_.getPoints(aimPoint, MSpace::kWorld);
        //ѭ����ǰ֡�е���������
        for (int pn = 0; pn < this->DoodleConvert.nV; pn++)
        {
            this->DoodleConvert.v.col(pn).segment<3>(3 * i) << aimPoint[pn][0], aimPoint[pn][1], aimPoint[pn][2];
        }
    }

    this->DoodleConvert.fStart(1) = this->DoodleConvert.fStart(0) + this->DoodleConvert.nF;
    // ��������������
    for (int s = 0; s < this->DoodleConvert.nS; s++) {
        for (int k = this->DoodleConvert.fStart(s); k < this->DoodleConvert.fStart(s + 1); k++)
        {
            this->DoodleConvert.subjectID(k) = s;
        }
    }

    //��ð�֡
    MDataHandle inputBindFrameHandle = dataBlock.inputValue(DoodleConvertBone::bindFrame, &status);
    int bindframe = inputBindFrameHandle.asInt64();
    //����Ƿ��ð�֡
    if (bindframe != this->__bindFrame__) {
        this->IsGetFrame = false;
        this->__bindFrame__ = bindframe;
    }
    if ((int)currentframe == bindframe)
    {
        MGlobal::displayInfo("�ѻ�ð�֡");
        this->IsGetFrame = true;
        this->DoodleConvert.u.resize(this->DoodleConvert.nS * 3, this->DoodleConvert.nV);
        for (int i = 0; i < this->DoodleConvert.nV; i++)
        {
            this->DoodleConvert.u.col(i) << aimPoint[i][0], aimPoint[i][1], aimPoint[i][2];
        }
    }



    if (this->IsGetFrame && this->getFrame.size() == (int)(endframe_ - startframe_ + 1)) {
        if (plug == subjectIndex)
        {
            MComputation computtation;
            computtation.beginComputation();

            MGlobal::displayInfo("��ʼ����ֲ�����......��ȴ�");
            this->DoodleConvert.init();
            if (computtation.isInterruptRequested()) {
                return MS::kFailure;
            }

            MGlobal::displayInfo("��ʼ����Ȩ��......��ȴ�");
            this->DoodleConvert.compute();

            if (computtation.isInterruptRequested()) {
                return MS::kFailure;
            }

            //��ü������
            int s = 0;
            Eigen::MatrixXd lr, lt, gb, lbr, lbt;
            this->DoodleConvert.computeRTB(s, lr, lt, gb, lbr, lbt);


            //�������
            MDataHandle outgetFrameHandle = dataBlock.outputValue(DoodleConvertBone::subjectIndex, &status);
            outgetFrameHandle.set(s);
            MArrayDataHandle outArrayAimRotationHandle = dataBlock.outputArrayValue(DoodleConvertBone::aimRotation, &status);
            MArrayDataBuilder outArrayAimRotationBuilder = outArrayAimRotationHandle.builder();
            for (int i = 0; i < this->DoodleConvert.nB; i++)
            {
                MArrayDataHandle aimpPintRotation = outArrayAimRotationBuilder.addElementArray(i);
                for (int it_ = 0; it_ < (int)(endframe_ - startframe_ + 1); it_++)
                {
                    MVector pointRotation = aimpPintRotation.builder().addElement(it_).asVector();
                    pointRotation.x = lr.col(i).segment<3>(3 * it_)[0];
                    pointRotation.y = lr.col(i).segment<3>(3 * it_)[1];
                    pointRotation.z = lr.col(i).segment<3>(3 * it_)[2];
                }
            }
            //outAimRotationHandle.set(MMatrix(lr));

            computtation.endComputation();
            CHECK_MSTATUS(dataBlock.setClean(plug))
        }
        else
        {
            return MS::kUnknownParameter;
        }

    }
    else
    {
        if (this->IsGetFrame)MGlobal::displayError("û�л�ð�֡, �벥�Ŷ���, ���ռ������Ͱ�֡");
        if (this->getFrame.size() == (int)(endframe_ - startframe_ + 1))MGlobal::displayError("û�л�ö�������, �벥�Ŷ���, ���ռ������Ͱ�֡");
        return MS::kUnknownParameter;
    }
    return MS::kUnknownParameter;
}


MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, "8.5", "Any");

    status = plugin.registerCommand("DoodleConvertBone", DoodleConvertBone::creator, DoodleConvertBone::createSyntax);
    if (!status) {
        status.perror("registerCommand");
        return status;
    }

    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterCommand("DoodleConvertBone");
    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    return status;
}

DoodleDemBones::DoodleDemBones()
{
}

DoodleDemBones::~DoodleDemBones()
{
}
