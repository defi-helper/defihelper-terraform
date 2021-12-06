locals {
    swagger = {
        certificate = {
            crt = var.ssl_1iu_ru_crt
            key = var.ssl_1iu_ru_key
        }
        ingress = {
            hosts = ["swagger.${var.cluster_domain}"]
            tls = [
                {
                    secretName = "swagger-https-certificate"
                    hosts = ["swagger.${var.cluster_domain}"]
                }
            ]
        }
    }
    le_annotations = [
        {
            name  = "ingress.annotations.cert-manager\\.io/cluster-issuer"
            value = "letsencrypt-production"
        }
    ]
    helm_chart_path = "${path.root}/modules/swagger/helm/swagger"
}

resource "helm_release" "swagger-ui" {
  name             = "swagger"
  namespace        = "services"
  chart            = local.helm_chart_path
  create_namespace = true
  atomic           = true
  values           = [yamlencode(local.swagger)]
  depends_on       = [var.dep]
  
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