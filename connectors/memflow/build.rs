fn main() {
    println!("cargo:rustc-link-search=../../build/x64/Release");
    println!("cargo:rustc-link-lib=static=hyperv-core");
}