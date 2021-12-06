resource "kubectl_manifest" "cluster-role-ingress" {
    yaml_body = file("${path.root}/modules/service-account/cluster-role-ingress.yaml")
}

resource "kubectl_manifest" "cluster-role-bindings-ingress-at" {
    yaml_body = file("${path.root}/modules/service-account/cluster-role-bindings-ingress-at.yaml")
}

resource "kubectl_manifest" "cluster-role-bindings-ingress-ok" {
    yaml_body = file("${path.root}/modules/service-account/cluster-role-bindings-ingress-ok.yaml")
}