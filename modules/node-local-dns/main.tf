resource "kubectl_manifest" "node-local-dns-service-account" {
   count      = var.enable_node_local_dns ? 1 : 0
   yaml_body = file("${path.root}/modules/node-local-dns/01-service-account.yaml")
}

resource "kubectl_manifest" "node-local-dns-service" {
   count      = var.enable_node_local_dns ? 1 : 0
   yaml_body = file("${path.root}/modules/node-local-dns/02-service.yaml")
}

resource "kubectl_manifest" "node-local-dns-configmap" {
   count      = var.enable_node_local_dns ? 1 : 0
   yaml_body = file("${path.root}/modules/node-local-dns/03-configmap.yaml")
}

resource "kubectl_manifest" "node-local-dns-daemonset" {
   count      = var.enable_node_local_dns ? 1 : 0
   yaml_body = file("${path.root}/modules/node-local-dns/04-daemonset.yaml")
}

resource "kubectl_manifest" "node-local-dns-service-local-dns" {
   count      = var.enable_node_local_dns ? 1 : 0
   yaml_body = file("${path.root}/modules/node-local-dns/05-service-local-dns.yaml")
}
