locals {
  pgadmin4 = {
    nodeSelector = var.node_selector
    "serverDefinitions" = {
      "enabled" = true

      "servers" = "\"1\": {\n  \"Name\": \"postgres\",\n  \"Group\": \"Servers\",\n  \"Port\": 6432,\n  \"Username\": \"development\",\n  \"Host\": \"rc1a-bs264y539el89c1w.mdb.yandexcloud.net\",\n  \"SSLMode\": \"prefer\",\n  \"MaintenanceDB\": \"postgres\"\n}  "
    }

    "livenessProbe" = {
      "failureThreshold"    = 3
      "initialDelaySeconds" = 60
      "periodSeconds"       = 60
      "successThreshold"    = 1
      "timeoutSeconds"      = 15
    }

    "strategy" = {
      "type" = "Recreate"
    }
  }



  le_annotations = [
    {
      name  = "ingress.annotations.cert-manager\\.io/cluster-issuer"
      value = "letsencrypt-production"
    }
  ]
}

# resource "kubernetes_secret" "https-certificate" {
#   metadata {
#     name      = "https-certificate"
#     namespace = "services"
#   }

#   data = {
#     "tls.crt" = var.ssl_1iu_ru_crt
#     "tls.key" = var.ssl_1iu_ru_key
#   }
# }

resource "helm_release" "pgadmin4" {
  name       = "pgadmin4"
  namespace  = "services"
  create_namespace = "true"
  repository = "https://helm.runix.net/"
  chart      = "pgadmin4"
  version    = "1.6.1"
  timeout    = 1800
  #values     = [file("modules/pgadmin4/values.yaml")]
  values     = [yamlencode(local.pgadmin4)]
  depends_on = [var.dep]


  # set {
  #   name  = "nodeSelector"
  #   value = "service"
  # }
  set {
    name  = "ingress.enabled"
    value = "true"
  }

  set {
    name  = "ingress.hosts[0].host"
    value = "pgadmin-k8s.${var.cluster_domain}"
  }

  set {
    name  = "ingress.hosts[0].paths[0].path"
    value = "/"
  }

  set {
    name  = "ingress.hosts[0].paths[0].pathType"
    value = "ImplementationSpecific"
  }

  set {
    name  = "ingress.tls[0].secretName"
    value = "https-certificate"
  }

  set {
    name  = "ingress.tls[0].hosts[0]"
    value = "pgadmin-k8s.${var.cluster_domain}"
  }

  set {
    name  = "env.password"
    value = var.pgadmin4_admin_password
  }

  set {
    name  = "env.email"
    value = var.pgadmin4_admin_email
  }

  set {
    name  = "persistentVolume.enabled"
    value = "true"
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
