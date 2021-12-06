locals {
  crds = [
    "crd-alertmanagerconfigs.yaml",
    "crd-alertmanagers.yaml",
    "crd-podmonitors.yaml",
    "crd-probes.yaml",
    "crd-prometheuses.yaml",
    "crd-prometheusrules.yaml",
    "crd-servicemonitors.yaml",
    "crd-thanosrulers.yaml"
  ]
}

resource "kubectl_manifest" "crds" {
   count = length(local.crds)
  yaml_body = file("${path.root}/modules/prometheus/modules/crds/${local.crds[count.index]}")
}
