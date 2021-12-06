locals {
  ingress_json = {
    for name, config in var.configs:
      name => lookup(config, "ingress", false) != false ? jsonencode({
        enabled = true
        hostname = config["ingress"]["domain"]
        tls = [
          {
            secretName = config["ingress"]["domain"]
            hosts = [config["ingress"]["domain"]]
          }
        ]
        annotations = merge({
          "kubernetes.io/ingress.class" = "nginx"
          "cert-manager.io/cluster-issuer" = config["ingress"]["issuer"]
          "nginx.ingress.kubernetes.io/use-regex" = "true"
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
    user = {
      create = true
      email = var.sentry_username
      password = var.sentry_password
    }
    ingress = local.ingress["sentry"]
    postgresql = {
      enabled = false
    }
    externalPostgresql = {
      host = var.pg_host
      port = var.pg_port
      username = var.pg_sentry_user_name
      password = var.pg_sentry_user_password
      database = "sentry"
    }
    rabbitmq = {
      replicaCount = 1
    }
    kafka = {
      replicaCount = 1
      defaultReplicationFactor = 1
      offsetsTopicReplicationFactor = 1
      transactionStateLogReplicationFactor = 1
    }
    relay ={
      nodeSelector = var.node_selector
    }
    clickhouse = {
      clickhouse = {
        replicas = 1
      }
    }
    redis = {
      cluster = {
        enabled = false
      }
      master = {
        nodeSelector = var.node_selector
      }
    }
    sentry = {
      web = {
        nodeSelector = var.node_selector
      }
      worker = {
        replicas = 1
        nodeSelector = var.node_selector
      }
      ingestConsumer = {
        nodeSelector = var.node_selector
      }
      cron = {
        nodeSelector = var.node_selector
      }
      postProcessForward = {
        nodeSelector = var.node_selector
      }
    }
    snuba = {
      api = {
        nodeSelector = var.node_selector
      }
      consumer = {
        nodeSelector = var.node_selector
      }
      outcomesConsumer = {
        nodeSelector = var.node_selector
      }
      replacer = {
        nodeSelector = var.node_selector
      }
      sessionsConsumer = {
        nodeSelector = var.node_selector
      }
      transactionsConsumer = {
        nodeSelector = var.node_selector
      }
    }
    hooks = {
      clickhouseInit = {
        nodeSelector = var.node_selector
      }
      dbCheck = {
        nodeSelector = var.node_selector
      }
      dbInit = {
        nodeSelector = var.node_selector
      }
      snubaInit = {
        nodeSelector = var.node_selector
      }
    }
    symbolicator = {
      nodeSelector = var.node_selector
    }
  }
}

resource "helm_release" "sentry" {
  count      = var.enable_sentry ? 1 : 0
  name = "sentry"
  repository = "https://sentry-kubernetes.github.io/charts"
  chart = "sentry"
  version = "11.5.1"
  namespace  = "sentry"
  create_namespace = true
  timeout = 600
  values = [yamlencode(local.values)]
  depends_on        = [var.dep]
}
