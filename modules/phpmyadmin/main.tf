locals {
  phpmyadmin = {
    nodeSelector = var.node_selector
  }



  le_annotations = [
    {
      name  = "ingress.annotations.cert-manager\\.io/cluster-issuer"
      value = "letsencrypt-production"
    }
  ]
}


resource "helm_release" "phpmyadmin" {
  name       = "phpmyadmin"
  namespace  = "services"
  repository = "https://charts.bitnami.com/bitnami"
  chart      = "phpmyadmin"
  values     = [yamlencode(local.phpmyadmin)]

  set {
    name  = "ingress.hostname"
    value = "phpmyadmin-k8s.${var.cluster_domain}"
  }

  set {
    name  = "ingress.enabled"
    value = "true"
  }

  set {
    name  = "ingress.tls"
    value = "true"
  }
  set {
    name  = "ingress.secrets[0].certificate"
    value = var.ssl_1iu_ru_crt

  }

  set {
    name  = "ingress.secrets[0].key"
    value = var.ssl_1iu_ru_key

  }

  set {
    name  = "ingress.secrets[0].name"
    value = "phpmyadmin-k8s.${var.cluster_domain}-tls"

  }
  dynamic "set" {
    for_each = [for a in local.le_annotations : {
      name  = a.name
      value = a.value
    }]
    content {
      name  = set.value.name
      value = set.value.value
    }
  }

}
