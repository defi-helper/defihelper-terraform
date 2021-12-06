locals {
  ingress_json = {
    for name, config in var.configs:
      name => lookup(config, "ingress", false) != false ? jsonencode({
        enabled = true
        hosts = [{
          "host" = config["ingress"]["domain"]
          "paths" = ["/"]
        }]
        tls = [
          {
            secretName = config["ingress"]["domain"]
            hosts = [config["ingress"]["domain"]]
          }
        ]
        annotations = merge({
          "kubernetes.io/ingress.class" = "nginx"
          "cert-manager.io/cluster-issuer" = config["ingress"]["issuer"]
        }, jsondecode(lookup(config, "http_auth", true) != false ? jsonencode({
          "nginx.ingress.kubernetes.io/auth-type" = "basic"
          "nginx.ingress.kubernetes.io/auth-realm" = "Authentication Required"
        }) : jsonencode({})))
      }) : jsonencode({})
  }
  ingress = {
    for name, json in local.ingress_json:
      name => jsondecode(json)
  }

  values = {
    nodeSelector = var.configs["centrifugo"].node_selector
    ingress = local.ingress["centrifugo"]
    metrics = {
      enabled = true
      serviceMonitor = {
        enabled = true
      }
    }
    replicaCount        = var.centrifugo_replicaCount
    config = {
      engine = "redis"
      redis_cluster_addrs = "${var.redis_centrifugo_host}:6379"
      admin = false
      web = true
      namespaces = [
        {
          name = "webinar"
          presence = true
          publish = true
          history_size = 50
          history_lifetime = 300
          history_recover = true
        },
        {
          name = "chat"
          presence = true
          publish = true
          history_size = 50
          history_lifetime = 60
          history_recover = true
        },
        {
          name = "at_chat"
          presence = true
          publish = true
          history_size = 50
          history_lifetime = 60
          history_recover = true
        },
        {
          name = "at_notification"
          presence = true
          publish = true
          history_size = 10
          history_lifetime = 60
          history_recover = true
        },
        {
          name = "ok_notification"
          presence = true
          history_size = 10
          history_lifetime = 60
        }
      ]
    }
    secrets = {
      redisPassword = var.redis_password
      adminPassword = var.centrifugo_admin_password
      adminSecret = var.centrifugo_admin_secret
      apiKey = var.centrifugo_api_key
      tokenHmacSecretKey = var.centrifugo_api_key
    }
    resources = {
      requests = {
        cpu     = "200m"
        memory  = "256Mi"
      }
      limits = {
        cpu     = "500m"
        memory  = "512Mi"
      }
    }
  }
}

resource "helm_release" "centrifugo" {
  name              = "centrifugo"
  repository        = "https://centrifugal.github.io/helm-charts"
  chart             = "centrifugo"
  namespace         = "centrifugo"
  create_namespace  = true
  version           = "6.1.0"
  values            = [yamlencode(local.values)]
  depends_on        = [var.dep]
}
