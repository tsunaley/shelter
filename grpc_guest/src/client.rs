use tonic::transport::Channel;
use crate::generated::attestation_agent::attestation_agent_service_client::AttestationAgentServiceClient;
use crate::generated::attestation_agent::{GetEvidenceRequest};

use crate::generated::attestation::attestation_service_client::AttestationServiceClient;
use crate::generated::attestation::{AttestationRequest, AttestationResponse};

use crate::generated::reference::reference_value_provider_service_client::ReferenceValueProviderServiceClient;
use crate::generated::reference::{ReferenceValueRegisterRequest};

pub mod generated {
    pub mod attestation_agent {
        include!(concat!(env!("CARGO_MANIFEST_DIR"), "/src/generated/attestation_agent.rs"));
    }
    pub mod attestation {
        include!(concat!(env!("CARGO_MANIFEST_DIR"), "/src/generated/attestation.rs"));
    }
    pub mod reference {
        include!(concat!(env!("CARGO_MANIFEST_DIR"), "/src/generated/reference.rs"));
    }
}

// 创建 Attestation Agent 客户端
pub async fn create_attestation_agent_client() -> Result<AttestationAgentServiceClient<Channel>, Box<dyn std::error::Error>> {
    AttestationAgentServiceClient::connect("http://192.168.50.10:50002").await.map_err(|e| Box::new(e) as _)
}

// 创建 Attestation Service 客户端
pub async fn create_attestation_service_client() -> Result<AttestationServiceClient<Channel>, Box<dyn std::error::Error>> {
    AttestationServiceClient::connect("http://localhost:50004").await.map_err(|e| Box::new(e) as _)
}

// 创建 Reference Value Provider 客户端
pub async fn create_reference_value_provider_client() -> Result<ReferenceValueProviderServiceClient<Channel>, Box<dyn std::error::Error>> {
    ReferenceValueProviderServiceClient::connect("http://127.0.0.1:50003").await.map_err(|e| Box::new(e) as _)
}

// 从 Attestation Agent 获取证据
pub async fn get_evidence(client: &mut AttestationAgentServiceClient<Channel>, runtime_data: Vec<u8>) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
    // 构建 GetEvidenceRequest 请求
    let request = tonic::Request::new(GetEvidenceRequest {
        runtime_data,
    });

    // 发送请求并获取响应
    let response = client.get_evidence(request).await?.into_inner();

    Ok(response.evidence)
}

// 调用 AttestationEvaluate 方法
pub async fn attestation_evaluate(client: &mut AttestationServiceClient<Channel>, request: AttestationRequest) -> Result<AttestationResponse, Box<dyn std::error::Error>> {
    let request = tonic::Request::new(request);
    let response = client.attestation_evaluate(request).await?.into_inner();
    Ok(response)
}

// 注册参考值
pub async fn register_reference_value(
    client: &mut ReferenceValueProviderServiceClient<Channel>,
    message: String
) -> Result<(), Box<dyn std::error::Error>> {
    let request = tonic::Request::new(ReferenceValueRegisterRequest { message });
    client.register_reference_value(request).await?;
    Ok(())
}