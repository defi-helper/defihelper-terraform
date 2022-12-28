locals {
  dependency = [
    resource.helm_release.istio-base,
    resource.helm_release.istio-discovery,
  ]
  namespaces = toset([
    "services",
    "filestorage",
    "webinar",
    "auth",
    "ok",
    "at",
  ])
  ingress_json = {
    for name, config in var.configs:
      name => lookup(config, "ingress", false) != false ? jsonencode({
        enabled = true
        hosts = [{
                 "host" = config["ingress"]["domain"]
                 "paths" = [{
                   "path" = "/"
                   "pathType" = "ImplementationSpecific"
                 }]
        }]
        tls = [
          {
            secretName = config["ingress"]["domain"]
            hosts = [config["ingress"]["domain"]]
          }
        ]
        annotations = merge({
          "kubernetes.io/ingress.class" = "nginx-ingress"
          "cert-manager.io/cluster-issuer" = config["ingress"]["issuer"]
        }, jsondecode(lookup(config, "http_auth", true) != false ? jsonencode({
          "nginx.ingress.kubernetes.io/auth-type" = "basic"
          "nginx.ingress.kubernetes.io/auth-secret" = kubernetes_secret.istio-basic-auth.metadata[0].name
          "nginx.ingress.kubernetes.io/auth-realm" = "Authentication Required"
        }) : jsonencode({})))
      }) : jsonencode({})
  }
  ingress = {
    for name, json in local.ingress_json:
      name => jsondecode(json)
  }

  values = {
    kiali = {
      ingress = local.ingress["kiali"]
    }
    zipkin = {
      ingress = local.ingress["zipkin"]
    }
  }
}

resource "kubernetes_namespace" "istio-system" {
  metadata {
    name = "istio-system"
  }
}

resource "kubernetes_namespace" "ns-with-istio" {
  for_each = local.namespaces
  metadata {
    name = each.key
    labels = {
      istio-injection = "enabled"
    }
  }
  depends_on = [local.dependency]
}

resource "kubernetes_secret" "istio-basic-auth" {
  metadata {
    name = "istio-basic-auth"
    namespace = kubernetes_namespace.istio-system.metadata[0].name
  }
  data = {
    auth = var.istio_auth
  }
  type = "Opaque"
}


resource "helm_release" "istio-base" {
  name       = "istio-base"
  chart      = "${path.root}/modules/istio/helm/istio/manifests/charts/base"
  namespace  = kubernetes_namespace.istio-system.metadata[0].name
  depends_on        = [var.dep]
}

resource "helm_release" "istio-discovery" {
  name       = "istio-discovery"
  chart      = "${path.root}/modules/istio/helm/istio/manifests/charts/istio-control/istio-discovery"
  namespace  = kubernetes_namespace.istio-system.metadata[0].name
  depends_on  = [helm_release.istio-base,]
}

resource "helm_release" "istio-egress" {
  name       = "istio-egress"
  chart      = "${path.root}/modules/istio/helm/istio/manifests/charts/gateways/istio-egress"
  namespace  = kubernetes_namespace.istio-system.metadata[0].name
  depends_on  = [helm_release.istio-discovery,]
}

resource "helm_release" "istio-kiali" {
  name       = "istio-kiali"
  chart      = "${path.root}/modules/istio/helm/kiali"
  namespace  = kubernetes_namespace.istio-system.metadata[0].name
  depends_on  = [helm_release.istio-discovery,]
  values      = [yamlencode(local.values.kiali)]
}

#resource "helm_release" "istio-zipkin" {
#  name       = "istio-zipkin"
#  chart      = "${path.root}/modules/istio/helm/zipkin"
#  namespace  = kubernetes_namespace.istio-system.metadata[0].name
#  depends_on  = [helm_release.istio-discovery,]
#  values      = [yamlencode(local.values.zipkin)]
#}
