# data "http" "crds_manifest" {
#   url = "https://github.com/jetstack/cert-manager/releases/download/v1.3.0/cert-manager.crds.yaml"
# }

locals {
  crds = [
    "certificaterequests.yaml",
    "certificates.yaml",
    "challenges.yaml",
    "clusterissuers.yaml",
    "issuers.yaml",
    "orders.yaml"
  ]
}

resource "kubectl_manifest" "crds" {
   count = length(local.crds)
  yaml_body = file("${path.root}/modules/cert-manager/modules/crds/${local.crds[count.index]}")
}
