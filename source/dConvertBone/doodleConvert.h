#include <Eigen/Dense>
#include <DemBones/DemBonesExt.h>
#include <maya>
class DoodleDemBones : public Dem::DemBonesExt<double, float> {
public:
	DoodleDemBones( );
	~DoodleDemBones( );
	//��ʼ����ÿ�ηָ�Ǵ�֮ǰ���ûص�������
	void cbInitSplitBegin( ) override;
	//��ʼ����ÿ�ζԹǴؽ��зָ�󶼻���ûص�����
	void cbInitSplitEnd( ) override;
	//��ÿ��ȫ�ֵ�������֮ǰ���ûص�����
	void cbIterBegin( ) override;
	//��ÿ��ȫ�ֵ������º���ûص��������������true����ֹͣ������
	bool cbIterEnd( ) override;
	//��ÿ�����Ȩ�ظ���֮ǰ���õĻص�����
	void cbWeightsBegin( ) override;
	//ÿ����ƤȨ�ظ��º���õĻص�����
	void cbWeightsEnd( ) override;
	//��ÿ�ι���ת������֮ǰ���õĻص�����
	void cbTranformationsBegin( ) override;
	// ÿ�ι���ת�����º���õĻص�����
	void cbTransformationsEnd( ) override;

	// ÿ���ֲ�����ת�����µ�������õĻص��������������true����ֹͣ����
	void cbWeightsIterBegin( ) override;
	//	��ÿ���ֲ�Ȩ�ظ��µ�������õĻص��������������true����ֹͣ������
	bool cbWeightsIterEnd( ) override;
private:
	MComputation dolCom;
};