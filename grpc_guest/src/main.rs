mod client; 

use serde::{Deserialize, Serialize};
use serde_json;
use base64::engine::general_purpose::URL_SAFE_NO_PAD;
use base64::Engine;

use crate::client::{create_attestation_agent_client, get_evidence};
use crate::client::{create_attestation_service_client, attestation_evaluate};
use crate::client::{create_reference_value_provider_client, register_reference_value};

#[derive(Serialize, Deserialize, Debug)]
pub struct Message {
    #[serde(default = "default_version")]
    version: String,
    payload: String,
    #[serde(rename = "type")]
    r#type: String,
}

fn default_version() -> String {
    "0.1.0".into()
}

pub mod generated {
    pub mod attestation_agent {
        include!(concat!(env!("CARGO_MANIFEST_DIR"), "/src/generated/attestation_agent.rs"));
    }
    pub mod attestation {
        include!(concat!(env!("CARGO_MANIFEST_DIR"), "/src/generated/attestation.rs"));
    }
    //新增
    pub mod reference {
        include!(concat!(env!("CARGO_MANIFEST_DIR"), "/src/generated/reference.rs"));
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {

    // 调用注册参考值的函数
    if let Err(e) = register_reference_value_from_sample().await {
        eprintln!("sample参考值注册失败: {:?}", e);
    }
    
    // 连接AA并获取evidence
    match create_attestation_agent_client().await {
        Ok(mut attestation_agent_client) =>{
            println!("与AA连接成功: {:?}", attestation_agent_client);

            let reference_value = "reference-value-1";
            let runtime_data = reference_value.as_bytes().to_vec();
        
            // 获取证据，并添加条件判断
            match get_evidence(&mut attestation_agent_client, runtime_data.clone()).await {
                Ok(evidence) => {
                    // 获取证据成功，输出证据
                    println!("Evidence from Attestation Agent: {:?}", evidence);
                    match create_attestation_service_client().await {
                        Ok(mut attestation_client) => {
                            println!("与AS连接成功: {:?}", attestation_client);

                            // 将 runtime_data 转换为 Base64编码
                            let runtime_data_base64 = base64::engine::general_purpose::URL_SAFE_NO_PAD.encode(&runtime_data);
                            // 构建 AttestationRequest
                            let request = generated::attestation::AttestationRequest {
                                tee: "sample".to_string(),  // TEE类型
                                evidence: base64::engine::general_purpose::URL_SAFE_NO_PAD.encode(&evidence),
                                runtime_data_hash_algorithm: "sha384".to_string(),  // 默认为sha384
                                init_data_hash_algorithm: "sha384".to_string(),    // 默认为sha384
                                policy_ids: vec![],  // Policy ID
                                runtime_data: Some(generated::attestation::attestation_request::RuntimeData::RawRuntimeData(runtime_data_base64)),
                                init_data: None,  // 初始化数据
                            };

                            // 调用attestation_evaluate进行验证
                            match attestation_evaluate(&mut attestation_client, request).await {
                                Ok(response) => {
                                    println!("Attestation 验证成功: {:?}", response.attestation_token);
                                }
                                Err(e) => {
                                    eprintln!("Attestation 验证失败: {:?}", e);
                                }
                            }
                        }
                        Err(e) => {
                            // 连接失败，输出错误消息
                            eprintln!("创建客户端失败: {:?}", e);
                        }
                    }
                }
                Err(e) => {
                    // 获取证据失败，输出错误消息
                    eprintln!("获取证据失败: {:?}", e);
                }
            }
        }
        Err(e) => {
            // 连接失败，输出错误消息
            eprintln!("创建客户端失败: {:?}", e);
        }
    }
    Ok(())
}

// 创建并注册参考值的函数
async fn register_reference_value_from_sample() -> Result<(), Box<dyn std::error::Error>> {
    // 创建客户端实例
    let reference_value_provider_client_result = create_reference_value_provider_client().await;
    match reference_value_provider_client_result {
        Ok(mut reference_value_provider_client) => {
            // 连接成功，输出成功消息
            println!("与RVPS连接成功: {:?}", reference_value_provider_client);

            // 读取 sample 文件内容
            let provenance_data = std::fs::read_to_string("sample")
                .map_err(|e| format!("读取 sample 文件失败: {:?}", e))?;
            
            // 将文件内容编码为 Base64 字符串
            let encoded_provenance_data = URL_SAFE_NO_PAD.encode(provenance_data.as_bytes());

            // 创建 Message 实例
            let message = Message {
                version: "0.1.0".into(),
                payload: encoded_provenance_data,
                r#type: "sample".into(), // 根据需要设置合适的类型
            };

            // 将 Message 实例序列化为 JSON 字符串
            let message_json = serde_json::to_string(&message)
                .map_err(|e| format!("序列化消息失败: {:?}", e))?;

            // 注册参考值
            if let Err(e) = register_reference_value(&mut reference_value_provider_client, message_json).await {
                eprintln!("注册参考值失败: {:?}", e);
            } else {
                println!("参考值登记成功。");
            }
        }
        Err(e) => {
            // 连接失败，输出错误消息
            eprintln!("创建客户端失败: {:?}", e);
        }
    }
    Ok(())
}
