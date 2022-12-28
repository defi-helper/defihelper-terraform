locals {
  values = {
    extraArgs    = ["--token-ttl", "0"]
    nodeSelector = var.node_selector
    metricsScraper = {
      enabled = true
    }
    ingress = {
      enabled = true
      annotations = {
        "kubernetes.io/ingress.class"                  = "nginx-ingress"
        "kubernetes.io/tls-acme"                       = "true"
        "cert-manager.io/cluster-issuer"               = var.ingress.issuer
        "ingress.kubernetes.io/ssl-redirect"           = "true"
        "nginx.ingress.kubernetes.io/rewrite-target"   = "/"
        "nginx.ingress.kubernetes.io/backend-protocol" = "HTTPS"
      }
      hosts = [var.ingress.domain]
      tls = [
        {
          secretName = var.ingress.name
          hosts      = [var.ingress.domain]
        }
      ]
    }
  }
}

resource "helm_release" "kubernetes-dashboard" {
  name             = "kubernetes-dashboard"
  repository       = "https://kubernetes.github.io/dashboard/"
  chart            = "kubernetes-dashboard"
  version          = "6.0.2"
  namespace        = "kubernetes-dashboard"
  create_namespace = true

  values = [yamlencode(local.values)]
}
