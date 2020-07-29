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
#include <maya/MItMeshVertex.h>
#include <maya/MDGContextGuard.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnMeshData.h>
#include <maya/MDagPathArray.h>

#include <Eigen/Dense>
#include <DemBones/DemBonesExt.h>

#include "DoodleConvertBone.h"

#define DIVISION(a) if(a) cout << "====================================================================================================" <<endl;
// CONSTRUCTOR:
DoodleConvertBone::DoodleConvertBone( )
{
    this->__bindFrame__ = 0;
    this->IsGetFrame = false;

    this->startFrame = 1;
    this->endFrame = 5;
    this->inputMesh = "";
    this->nBones = 30;
    this->objName = "doodle";

    this->nInitIters = 10;
    this->nIters = 30;
    this->nTransIters = 5;
    this->isBindUpdate = 0;
    this->transAffine = 10;
    this->transAffineNorm = 4;
    this->nWeightsIters = 3;
    this->nonZeroWeightsNum = 8;
    this->weightsSmooth = 0.0001;
    this->weightsSmoothStep = 0.5;

    this->subjectIndex = 0;

}

// DESTRUCTOR:
DoodleConvertBone::~DoodleConvertBone( )
{
}

// FOR CREATING AN INSTANCE OF THIS COMMAND:
void* DoodleConvertBone::creator( )
{
    return new DoodleConvertBone;
}

MStatus DoodleConvertBone::AnalysisCommand(MArgList arge, MStatus Doolstatus)
{
    int bindFrame;
    //�����ĵ���ʱ��д
    MArgDatabase argData(syntax( ), arge, &Doolstatus);
    if (argData.isFlagSet("help")) {
        MGlobal::displayInfo("����ʲô��û��");
        return MS::kSuccess;
    }

    //���Կ�ʼ֡�ͽ���֡
    if (argData.isFlagSet("startFrame"))
    {
        argData.getFlagArgument("startFrame", 0, this->startFrame);
    }
    else
    {
        MGlobal::displayError("��ָ����ʼ֡");
        return MS::kFailure;
    }
    if (argData.isFlagSet("endFrame")) { argData.getFlagArgument("endFrame", 0, this->endFrame); }
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
    this->DoodleConvert.nF = this->endFrame - this->startFrame;
    return Doolstatus;
}

MStatus DoodleConvertBone::InitAndCimpute( )
{
    MComputation computtation;
    computtation.beginComputation( );

    MGlobal::displayInfo("��ʼ����ֲ�����......��ȴ�");
    this->DoodleConvert.init( );
    if (computtation.isInterruptRequested( )) {
        return MS::kFailure;
    }

    MGlobal::displayInfo("��ʼ����Ȩ��......��ȴ�");
    this->DoodleConvert.compute( );
    computtation.endComputation( );
    return MStatus::kSuccess;
}

MSyntax DoodleConvertBone::createSyntax( )
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
    MStatus Doolstatus = MStatus::kSuccess;
    
    // ����һЩ�������
    CHECK_MSTATUS_AND_RETURN_IT(this->AnalysisCommand(arge, Doolstatus));
    //������ֻ��һ�������ת��
    MSelectionList sel;
    CHECK_MSTATUS(sel.add(this->inputMesh));
    //��� dag path
    MDagPath inputMeshPath;
    sel.getDagPath(0, inputMeshPath);
    // ��dag·��ת����������
    CHECK_MSTATUS(inputMeshPath.extendToShape( ))
        if (inputMeshPath.apiType( ) != MFn::Type::kMesh)
        {
            MGlobal::displayError("������������");
            return MS::kFailure;
        }

    //���������
    MFnMesh inputMesh_(inputMeshPath.node( ));
    this->objName = inputMesh_.name( );
    // ��ʼ��һЩ�������ݵ�����
    this->initConvertAttr(inputMesh_);
    // ���������������
    this->GetMeshData(inputMeshPath);

    this->SetSubObjectIndex( );

    if (this->IsGetFrame) {

        CHECK_MSTATUS_AND_RETURN_IT(this->InitAndCimpute( ));
        // ��ü������
        this->DoodleConvert.computeRTB(subjectIndex,
                                       localRotation,
                                       localTranslation,
                                       globalBindMatrices,
                                       localBindPoseRotation,
                                       localBindPoseTranslation);
        // ���ø��ֽڵ�
        this->createJoins( );
        this->addCurve( );
        this->addSkinCluster( );

    }
    else
    {
        if (this->IsGetFrame)MGlobal::displayError("û�л�ð�֡, �벥�Ŷ���, ���ռ������Ͱ�֡");
        return MS::kFailure;
    }
    return MS::kSuccess;
}

const bool DoodleConvertBone::isHistoryOn( )
{
    return false;
}

const bool DoodleConvertBone::isUndoable( )
{
    return false;
}

void DoodleConvertBone::SetSubObjectIndex( )
{
    this->DoodleConvert.fStart(1) = this->DoodleConvert.fStart(0) + this->DoodleConvert.nF;
    // ��������������
    for (int s = 0; s < this->DoodleConvert.nS; s++) {
        for (int k = this->DoodleConvert.fStart(s); k < this->DoodleConvert.fStart(s + 1); k++)
        {
            this->DoodleConvert.subjectID(k) = s;
        }
    }
}

void DoodleConvertBone::initConvertAttr(MFnMesh& inputMesh_)
{
    //���þ�ֹ�Ķ�����
    this->DoodleConvert.nV = inputMesh_.numVertices( );
    //����һЩȫ��ֵ
    this->DoodleConvert.v.resize(3 * this->DoodleConvert.nF, this->DoodleConvert.nV);
    this->DoodleConvert.fTime.resize(this->DoodleConvert.nF);
    this->DoodleConvert.fStart.resize(this->DoodleConvert.nS + 1);
    this->DoodleConvert.fStart(0) = 0;
    this->DoodleConvert.fv.resize(inputMesh_.numFaceVertices( ));
    this->DoodleConvert.subjectID.resize(this->DoodleConvert.nF);
}

void DoodleConvertBone::GetFrameMeshData(int frame, MObject& MobjMesh)
{
    /// <summary>
    /// ��ô���֡�����񶥵�����
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="MobjMesh"></param>
    this->DoodleConvert.fTime(frame) = frame;
    MItMeshVertex vexpoint(MobjMesh);
    // ʹ�õ�������������λ��
    for (vexpoint.reset( ); !vexpoint.isDone( ); vexpoint.next( ))
    {
        int index = vexpoint.index( );
        MPoint pos = vexpoint.position(MSpace::kWorld);
        this->DoodleConvert.v.col(index).segment<3>(3 * frame) << pos.x, pos.y, pos.z;
    }
}

void DoodleConvertBone::GetBindFrame(MObject& MobjMesh)
{
    /// <summary>
    /// ��������а�֡������
    /// </summary>
    /// <param name="MobjMesh"></param>


    MItMeshVertex vexpointBindFrame(MobjMesh);
    MGlobal::displayInfo("�ѻ�ð�֡");
    this->IsGetFrame = true;
    this->DoodleConvert.u.resize(this->DoodleConvert.nS * 3, this->DoodleConvert.nV);
    // ���ö������������;
    for (vexpointBindFrame.reset( ); !vexpointBindFrame.isDone( ); vexpointBindFrame.next( ))
    {
        int index = vexpointBindFrame.index( );
        MPoint pos = vexpointBindFrame.position(MSpace::kWorld);
        this->DoodleConvert.u.col(index).segment(0, 3) << pos.x, pos.y, pos.z;
    }
    // ��������polygon obj�Ķ���
    MItMeshPolygon vexIter(MobjMesh);
    for (vexIter.reset( ); !vexIter.isDone( ); vexIter.next( )) {
        int index = vexIter.index( );
        MIntArray vexIndexArray;
        vexIter.getVertices(vexIndexArray);

        std::vector<int> mindex;
        for (unsigned int vexindex = 0; vexindex < vexIndexArray.length( ); vexindex++)
        {
            mindex.push_back(vexIndexArray[vexindex]);
        }
        this->DoodleConvert.fv[index] = mindex;
    }
    //if (!MobjMesh.hasFn(MFn::kMesh))
    //{
    //    MGlobal::displayInfo("��������Ը���");
    //}
    //MFnMesh fnBindMesh;
    //MFnMeshData dataBindMesh(MobjMesh);
    //fnBindMesh.copy(dataBindMesh.object(), MObject::kNullObj);
    //this->bindObj = fnBindMesh.object( );
    //this->dgModidier.renameNode(this->bindObj, this->objName + "doodleConvert");
    //this->dgModidier.doIt( );
}

void DoodleConvertBone::GetMeshData(MDagPath& inputMeshPath)
{
    /// <summary>
    /// ���������������
    /// </summary>
    /// <param name="inputMeshPath"></param>
    //���dag �����ڵ�
    MFnDependencyNode inputmeshDepPathNode(inputMeshPath.node( ));
    MPlug meshPlugs = inputmeshDepPathNode.findPlug(MString("outMesh"));
    MObject MobjMesh;
    // ѭ���������Ͱ�֡
    for (int i = 0; i < (this->DoodleConvert.nF); i++)
    {
        int currentFrame = i + this->startFrame;
        // ���õ�ǰ֡��������
        MDGContext ctx(MTime(currentFrame, MTime::uiUnit( )));
        MDGContextGuard guard(ctx);
        // ��õ�ǰʱ���������
        meshPlugs.getValue(MobjMesh);
        //CHECK_MSTATUS_AND_RETURN_IT();
        //ѭ����ǰ֡�е���������
        MGlobal::displayInfo("�ѻ��" + MString( ) + (currentFrame)+"֡����");
        //���ý���֡
        GetFrameMeshData(i, MobjMesh);
        //��ð�֡
        if (currentFrame == this->__bindFrame__)
        {
            GetBindFrame(MobjMesh);
        }
    }
}

void DoodleConvertBone::createJoins()
{
    /// <summary>
    /// ��������
    /// </summary>
    /// <param name="name"></param>
    MStatus statu;
    this->doolJoint.clear( );
    int index = 0;
    for (int i = 0; i < this->DoodleConvert.nB; i++)
    {
        MString name;
        name.format("^1s_^2s_^3s", this->objName, MString("joint"), MString( ) + i);
        MFnIkJoint joint;
        MObject jointObject = joint.create( );
        this->dagModifier.renameNode(jointObject, name);
        this->dagModifier.doIt( );
        joint.setRotationOrder(MTransformationMatrix::RotationOrder::kXYZ, true);
        joint.setTranslation(MVector(this->localBindPoseTranslation.col(index).segment<3>(0)[0],
                             this->localBindPoseTranslation.col(index).segment<3>(0)[1],
                             this->localBindPoseTranslation.col(index).segment<3>(0)[2]), MSpace::kWorld);
        double ro[3] = { this->localBindPoseRotation.col(index).segment<3>(0)[0],
                          this->localBindPoseRotation.col(index).segment<3>(0)[1],
                          this->localBindPoseRotation.col(index).segment<3>(0)[2] };
        joint.setRotation(ro, MTransformationMatrix::RotationOrder::kXYZ);
        doolJoint.push_back(joint.object());
        index++;
    }
}

void DoodleConvertBone::addCurve( )
{
    /// <summary>
    /// ��������
    /// </summary>
    MFnAnimCurve aim;
    int index = 0;
    for (std::vector<MObject>::const_iterator i = this->doolJoint.begin( );
         i != this->doolJoint.end( ); ++i)
    {
        MFnIkJoint fnjoint(*i);
        MPlug plugtx = fnjoint.findPlug("tx");
        MPlug plugty = fnjoint.findPlug("ty");
        MPlug plugtz = fnjoint.findPlug("tz");
        MPlug plugrx = fnjoint.findPlug("rx");
        MPlug plugry = fnjoint.findPlug("ry");
        MPlug plugrz = fnjoint.findPlug("rz");

        MTimeArray timeArray;
        MDoubleArray doubleArrayTX;
        MDoubleArray doubleArrayTY;
        MDoubleArray doubleArrayTZ;
        MDoubleArray doubleArrayRX;
        MDoubleArray doubleArrayRY;
        MDoubleArray doubleArrayRZ;
        for (int Dtime = 0; Dtime < this->DoodleConvert.fTime.size( ); Dtime++)
        {
            int frame = this->DoodleConvert.fTime(Dtime) + this->startFrame;
            timeArray.append(MTime(frame, MTime::uiUnit( )));
            Eigen::Vector3d lt = this->localTranslation.col(index).segment<3>(3 * Dtime);
            //���ð�ƽ������
            doubleArrayTX.append(lt.x());
            doubleArrayTY.append(lt.y());
            doubleArrayTZ.append(lt.z());
            //���ð���ת����
            doubleArrayRX.append(this->localRotation.col(index).segment<3>(3 * Dtime).x());
            doubleArrayRY.append(this->localRotation.col(index).segment<3>(3 * Dtime).y());
            doubleArrayRZ.append(this->localRotation.col(index).segment<3>(3 * Dtime).z());
        }

        //ƽ������
        MObject aimTX = aim.create(plugtx, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &this->dgModidier);
        aim.addKeys(&MTimeArray(timeArray), &doubleArrayTX);
        MObject aimTY = aim.create(plugty, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &this->dgModidier);
        aim.addKeys(&MTimeArray(timeArray), &doubleArrayTY);
        MObject aimTZ = aim.create(plugtz, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &this->dgModidier);
        aim.addKeys(&MTimeArray(timeArray), &doubleArrayTZ);
        //��ת����
        MObject aimRX = aim.create(plugrx, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &this->dgModidier);
        aim.addKeys(&MTimeArray(timeArray), &doubleArrayRX);
        MObject aimRY = aim.create(plugry, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &this->dgModidier);
        aim.addKeys(&MTimeArray(timeArray), &doubleArrayRY);
        MObject aimRZ = aim.create(plugrz, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &this->dgModidier);
        aim.addKeys(&MTimeArray(timeArray), &doubleArrayRZ);
        
        //����������
        MString acTX;
        MString acTY;
        MString acTZ;
        MString acRX;
        MString acRY;
        MString acRZ;
        acTX.format("^1s_^2s_^3s", this->objName, "aimcurveTX", MString( ) + index);
        acTY.format("^1s_^2s_^3s", this->objName, "aimcurveTY", MString( ) + index);
        acTZ.format("^1s_^2s_^3s", this->objName, "aimcurveTZ", MString( ) + index);
        acRX.format("^1s_^2s_^3s", this->objName, "aimcurveRX", MString( ) + index);
        acRY.format("^1s_^2s_^3s", this->objName, "aimcurveRY", MString( ) + index);
        acRZ.format("^1s_^2s_^3s", this->objName, "aimcurveRZ", MString( ) + index);
        this->dgModidier.renameNode(aimTX, acTX);
        this->dgModidier.renameNode(aimTY, acTY);
        this->dgModidier.renameNode(aimTZ, acTZ);
        this->dgModidier.renameNode(aimRX, acRX);
        this->dgModidier.renameNode(aimRY, acRY);
        this->dgModidier.renameNode(aimRZ, acRZ);
        // ��������
        index++;
    }
}

void DoodleConvertBone::addSkinCluster( )
{   
    /// <summary>
    /// ���Ƥ����
    /// </summary>
    MStatus status;
    // ����Դ����
    MGlobal::viewFrame(MTime(this->__bindFrame__, MTime::uiUnit( )));
    MSelectionList sel;
    CHECK_MSTATUS(sel.add(this->inputMesh));
    //��� dag path
    MDagPath inputMeshPath;
    sel.getDagPath(0, inputMeshPath);
    MFnTransform inputTrans(inputMeshPath);
    this->bindObj = inputTrans.duplicate(false,false , &status);
    CHECK_MSTATUS(status);
    this->dgModidier.renameNode(this->bindObj, this->objName + "bindobj");
    MFnMesh fnbindObjMesh(MFnTransform(this->bindObj).child(0));
    MObject bindObjOrg = fnbindObjMesh.copy(MFnTransform(this->bindObj).child(0),this->bindObj);
    CHECK_MSTATUS(status);
    this->dgModidier.renameNode(bindObjOrg, this->objName + "bindobjOrg");
    this->dgModidier.doIt( );
    MFnMesh fnbindObjOrgMesh(bindObjOrg);
    CHECK_MSTATUS(fnbindObjOrgMesh.setIntermediateObject(true));

    //MString comm;
    //comm.format("^1s ^2s ^3s", MString("skinCluster -name"), this->objName + "skincluster", MString("-toSelectedBones"));
    //for (std::vector<MObject>::const_iterator joint = this->doolJoint.begin( ); joint != doolJoint.end( ); ++joint)
    //{
    //    MString jointstr;
    //    MFnIkJoint fnJoint(*joint);
    //    jointstr.format(" ^1s", fnJoint.fullPathName( ));
    //    comm += jointstr;
    //}
    //MString meshstr;
    //meshstr.format(" ^1s", MFnTransform(this->bindObj).fullPathName( ));
    //comm += meshstr;
    //MString skinCluster;
    // ִ������
    //this->dgModidier.commandToExecute()
    //CHECK_MSTATUS(MGlobal::executeCommandOnIdle(comm, true));
    //MSelectionList selSkinCluster;
    //CHECK_MSTATUS(selSkinCluster.add(this->objName + "skincluster"));
    //MDagPath dagSkinCluster;
    //selSkinCluster.getDagPath(0, dagSkinCluster);
    //MFnSkinCluster fnSkinCluster(dagSkinCluster.node());
    
    //MPlug weightList = fnSkinCluster.findPlug("weightList");
    //for (int i = 0; i < this->DoodleConvert.nV; i++)
    //{
    //    MPlug weightArray = weightList.child(i).child(0);
    //    int index = 0;
    //    for (std::vector<MObject>::const_iterator joint = this->doolJoint.begin( ); joint != doolJoint.end( ); ++joint)
    //    {
    //        weightArray.elementByLogicalIndex(index).setValue(this->DoodleConvert.w.coeff(index, i));
    //        cout << "��������" << index << " �������� " << i << " --" << this->DoodleConvert.w.coeff(index, i) << endl;
    //        index++;
    //    }
    //    
    //}
    //���Ƥ����
    MFnSkinCluster skinCluster;
    MObject skinClusterObj = skinCluster.create("skinCluster");
    this->dgModidier.renameNode(skinClusterObj, this->objName + "skinCluster");
    this->dgModidier.doIt( );
    
    // ���bindpose
    MFnDependencyNode bindpose;
    MObject bindPoseObj = bindpose.create("dagPose");
    this->dgModidier.renameNode(bindPoseObj, this->objName + "bindpose");
    this->dgModidier.doIt( );
    // ��� skinset
    MObject skinsetObj = this->dgModidier.createNode("objectSet");
    MFnDagNode skinset(skinClusterObj);
    this->dgModidier.renameNode(skinsetObj, this->objName + "skinset");
    this->dgModidier.doIt( );
    
    // ���bindmesh
    MFnMesh bindmesh(MFnTransform(this->bindObj).child(0));
    
    
    // ����bindpose��skincluster
    CHECK_MSTATUS(this->dgModidier.connect(bindpose.findPlug("message"), skinCluster.findPlug("bindPose")));
    // ����skincluster ��mesh , skinobjset
    CHECK_MSTATUS(this->dgModidier.connect(skinCluster.findPlug("outputGeometry").elementByLogicalIndex(0), bindmesh.findPlug("inMesh")));
    //CHECK_MSTATUS(this->dgModidier.connect(skinCluster.findPlug("message"), skinset.findPlug("usedBy").elementByLogicalIndex(0)));
    // ����mesh ��skinset
    /*CHECK_MSTATUS(this->dgModidier.connect(bindmesh.findPlug("instObjGroups").elementByLogicalIndex(0).elementByLogicalIndex(0).elementByLogicalIndex(0),
                  skinset.findPlug("dagSetMembers").elementByLogicalIndex(0)));*/
    // ����skinobjset ��mesh
    /*CHECK_MSTATUS(this->dgModidier.connect(skinset.findPlug("memberWireframeColor"),
                  bindmesh.findPlug("instObjGroups").elementByLogicalIndex(0).elementByLogicalIndex(0).elementByLogicalIndex(2)));*/


    
    CHECK_MSTATUS(this->dgModidier.doIt( ));
    // ׼������
    MPlug weightList = skinCluster.findPlug("weightList");
    int index = 0;
    for (std::vector<MObject>::const_iterator joint = this->doolJoint.begin(); joint != doolJoint.end(); ++joint)
    {
        MFnIkJoint fnJoint(*joint);
        // ���ӹ�����bindoise
        CHECK_MSTATUS(this->dgModidier.connect(fnJoint.findPlug("message"), bindpose.findPlug("members").elementByLogicalIndex(index)));
        CHECK_MSTATUS(this->dgModidier.connect(fnJoint.findPlug("bindPose"), bindpose.findPlug("worldMatrix").elementByLogicalIndex(index)));
        // ���ӹ�����Ƥ����
        //CHECK_MSTATUS(this->dgModidier.connect(fnJoint.findPlug("locklnfluenceWeights"), skinCluster.findPlug("lockWeights").elementByLogicalIndex(index)));
        CHECK_MSTATUS(this->dgModidier.connect(fnJoint.findPlug("worldMatrix").elementByLogicalIndex(0), skinCluster.findPlug("matrix").elementByLogicalIndex(index)));
        CHECK_MSTATUS(this->dgModidier.connect(fnJoint.findPlug("objectColorRGB"), skinCluster.findPlug("influenceColor").elementByLogicalIndex(index)));
        CHECK_MSTATUS(this->dgModidier.doIt( ));
        index++;
    }

    MFnDagNode fnbindObj(this->bindObj);
    MItMeshVertex vex(fnbindObj.child(0));
    for (vex.reset( ); !vex.isDone( ); vex.next( ))
    {
        int index = 0;
        for (std::vector<MObject>::const_iterator joint = this->doolJoint.begin( ); joint != doolJoint.end( ); ++joint)
        {
            double weight = this->DoodleConvert.w.coeff(index, vex.index( ));
            if (weight != 0.0) {
                CHECK_MSTATUS(skinCluster.setWeights(MFnDagNode(fnbindObj.child(0)).dagPath( ), vex.currentItem( ), index, weight, false));
            }
            index++;
        }
    }
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

DoodleDemBones::DoodleDemBones( )
{
}

DoodleDemBones::~DoodleDemBones( )
{
}
