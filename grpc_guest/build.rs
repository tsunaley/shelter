use tonic_build;

fn main() {
    // 确保 proto 目录存在
    if !std::fs::metadata("proto").is_ok() {
        panic!("proto directory does not exist");
    }

    // proto 文件列表
    let proto_files = [
        "proto/attestation-agent.proto",
        "proto/attestation.proto",  
        "proto/keyprovider.proto",  
        "proto/reference.proto",  
    ];

    // 确保每个 proto 文件存在
    for proto_file in &proto_files {
        if !std::fs::metadata(proto_file).is_ok() {
            panic!("Proto file {} does not exist", proto_file);
        }
    }

    println!("All proto files exist, proceeding to compile...");

    // // 获取 OUT_DIR 环境变量
    // let out_dir = env::var("OUT_DIR").unwrap();

    // 生成 Rust 代码并输出到指定目录
    tonic_build::configure()
        .out_dir("src/generated") // 生成的 Rust 文件的位置
        .compile(&proto_files, &["proto"]) // .proto 文件所在的目录
        .expect("Failed to compile protobuf files");

    println!("Protobuf files compiled successfully");
}
