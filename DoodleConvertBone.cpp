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
#include <vector>

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
#include <maya/MItMeshPolygon.h>


#include <Eigen/Dense>
#include <DemBones/DemBonesExt.h>

#include "DoodleConvertBone.h"

#define DIVISION(a) if(a) cout << "====================================================================================================" <<endl;
// CONSTRUCTOR:
DoodleConvertBone::DoodleConvertBone()
{
    this->__bindFrame__ = 0;
    this->IsGetFrame = false;
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
    int bindFrame;
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
    if (argData.isFlagSet("bindFrame")) { argData.getFlagArgument("bindFrame", 0, bindFrame); }
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
    
    //����Ƿ��ð�֡
    if (bindFrame != this->__bindFrame__) {
        this->IsGetFrame = false;
        this->__bindFrame__ = bindFrame;
    }

    this->DoodleConvert.nS = 1;
    //������֡��
    this->DoodleConvert.nF = endFrame - startFrame;

    //������ֻ��һ�������ת��
    MSelectionList sel;
    CHECK_MSTATUS(sel.add(inputMesh));
    //��� dag path
    MDagPath inputMeshPath;
    sel.getDagPath(0, inputMeshPath);
    // ��dag·��ת����������
    CHECK_MSTATUS(inputMeshPath.extendToShape())
    if (inputMeshPath.apiType() != MFn::Type::kMesh)
    { 

        MGlobal::displayError("������������"); 
        return MS::kFailure;
    }
    
    //���������
    MFnMesh inputMesh_(inputMeshPath.node());
    //���dag �����ڵ�
    MFnDependencyNode inputmeshDepPathNode(inputMeshPath.node());
    MPlug meshPlugs = inputmeshDepPathNode.findPlug(MString("outMesh"), &status);
    MObject test;
    
    
    //MFnMesh test( );
    //��ö���
    MPointArray aimPoint;
    inputMesh_.getPoints(aimPoint, MSpace::kWorld);
    //���þ�ֹ�Ķ�����
    this->DoodleConvert.nV = inputMesh_.numVertices();
    //����һЩȫ��ֵ
    this->DoodleConvert.v.resize(3 * this->DoodleConvert.nF, this->DoodleConvert.nV);
    this->DoodleConvert.fTime.resize(this->DoodleConvert.nF);
    this->DoodleConvert.fStart.resize(this->DoodleConvert.nS + 1);
    this->DoodleConvert.fStart(0) = 0;
    this->DoodleConvert.fv.resize(inputMesh_.numVertices());
    this->DoodleConvert.subjectID.resize(this->DoodleConvert.nF);
    //this->DoodleConvert.w = (Eigen::MatrixXd(0, 0)).sparseView(1, 1e-20);

    // ѭ���������Ͱ�֡
    for (int i = 0; i < (this->DoodleConvert.nF); i++)
    {
        int currentFrame = i + startFrame;
        // ���õ�ǰ֡
        MAnimControl::setCurrentTime(MTime(currentFrame, MTime::uiUnit()));
        // ��õ�ǰʱ���������
        CHECK_MSTATUS_AND_RETURN_IT(meshPlugs.getValue(test, MDGContext(MTime(currentFrame, MTime::uiUnit( )))));
        inputMesh_.getPoints(aimPoint, MSpace::kWorld);
        //ѭ����ǰ֡�е���������
        MGlobal::displayInfo("�ѻ��" + MString() + (currentFrame) + "֡����");

        this->DoodleConvert.fTime(i) = i;

        for (int pn = 0; pn < this->DoodleConvert.nV; pn++)
        {
            this->DoodleConvert.v.col(pn).segment<3>(3 * i) << aimPoint[pn][0], aimPoint[pn][1], aimPoint[pn][2];
        }
        //��ð�֡
        if (currentFrame == bindFrame)
        {

            MGlobal::displayInfo("�ѻ�ð�֡");
            this->IsGetFrame = true;
            this->DoodleConvert.u.resize(this->DoodleConvert.nS * 3, this->DoodleConvert.nV);
            // ���ö������������;
            DIVISION(debug)
            if (debug) cout << aimPoint << endl;
            DIVISION(debug)
            for (int pn = 0; pn < this->DoodleConvert.nV; pn++)
            {
                this->DoodleConvert.u.col(pn).segment(0,3) << aimPoint[pn][0], aimPoint[pn][1], aimPoint[pn][2];
                if (debug) cout << aimPoint[pn][0] << endl;
                if (debug) cout << aimPoint[pn][1] << endl;
                if (debug) cout << aimPoint[pn][2] << endl;
            }
            DIVISION(debug)
            // ��������polygon obj�Ķ���
            if (debug) cout << this->DoodleConvert.u << endl;
            DIVISION(debug)
            MItMeshPolygon vexIter(inputMeshPath);
            for (vexIter.reset(); !vexIter.isDone(); vexIter.next()) {
                int index = vexIter.index();
                MIntArray vexIndexArray;
                vexIter.getVertices(vexIndexArray);
                
                std::vector<int> mindex;
                for (unsigned int vexindex = 0; vexindex < vexIndexArray.length(); vexindex++)
                {
                    mindex.push_back(vexIndexArray[vexindex]);
                }
                this->DoodleConvert.fv[index] = mindex;
            }
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

    if (this->IsGetFrame) {
        MGlobal::displayInfo("ok");
        MComputation computtation;
        computtation.beginComputation();

        MGlobal::displayInfo("��ʼ����ֲ�����......��ȴ�");
        this->DoodleConvert.init();
        if (computtation.isInterruptRequested()) {
            return MS::kFailure;
        }

        MGlobal::displayInfo("��ʼ����Ȩ��......��ȴ�");
        //this->DoodleConvert.compute();
        try
        {
            //
        }
        catch (...)//const std::exception&
        {
            MGlobal::displayError("����ʧ��");
            return MS::kFailure;
        }
        

        if (computtation.isInterruptRequested()) {
            return MS::kFailure;
        }

        // ��ü������
        int s = 0;
        Eigen::MatrixXd lr, lt, gb, lbr, lbt;
        this->DoodleConvert.computeRTB(s, lr, lt, gb, lbr, lbt);
        //cout << s << endl;
        //cout << lr << endl;
        //cout << lt << endl;
        //cout << gb << endl;
        //cout << lbr << endl;
        //cout << lbt << endl;
        // �������
        for (int i = 0; i < this->DoodleConvert.nB; i++)
        {
            for (int it_ = 0; it_ < (int)(this->DoodleConvert.nF); it_++)
            {
                
            }
        }
        computtation.endComputation();
    }
    else
    {
        if (this->IsGetFrame)MGlobal::displayError("û�л�ð�֡, �벥�Ŷ���, ���ռ������Ͱ�֡");
        return MS::kFailure;
    }
    return MS::kSuccess;
}


MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, "0.1", "Any");

    status = plugin.registerCommand("doodleConvertBone", DoodleConvertBone::creator, DoodleConvertBone::createSyntax);
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

    status = plugin.deregisterCommand("doodleConvertBone");
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
