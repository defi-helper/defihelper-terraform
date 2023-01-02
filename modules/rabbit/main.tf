locals{
    rabbit = {
        global = {
          storageClass = "yc-network-ssd"
        }
        ingress = {
            enabled     = true
            hostname    = "${var.rabbitmq_host}.${var.cluster_domain}"
            certManager = true
            tls         = true
            tlsSecret   = "rabbitmq-tls"
        }
        volumePermissions = {
            enabled = true
        }
        auth = {
            password        = var.rabbitmq_password
            erlangCookie    = var.rabbitmq_erlangcookie
        }
        resources = {
            requests = {
                cpu     = "200m"
                memory  = "512Mi"
            }
            limits = {
                cpu     = "500m"
                memory  = "1024Mi"
            }
        }
        replicaCount        = var.rabbitmq_replicaCount
        communityPlugins    = "https://github.com/rabbitmq/rabbitmq-delayed-message-exchange/releases/download/3.10.2/rabbitmq_delayed_message_exchange-3.10.2.ez"
        extraPlugins        = "rabbitmq_delayed_message_exchange rabbitmq_shovel rabbitmq_shovel_management"
        nodeSelector = {
          group_name = "service"
        }
        metrics = {
          enabled = true
          serviceMonitor = {
            enabled = true
          }
        }
    }

    exporter = {
        rabbitmq = {
            url         = "http://rabbitmq.services.svc.cluster.local:15672"
            user        = "user"
            password    = var.rabbitmq_password
            capabilities = "bert,no_sort"
            include_queues = ".*"
            include_vhost = ".*"
            skip_queues = "^$"
            skip_verify = "false"
            skip_vhost = "^$"
            exporters = "exchange,node,overview,queue"
            output_format = "TTY"
            timeout = 30
            max_queues = 0
        }
        resources = {
            requests = {
                cpu     = "100m"
                memory  = "128Mi"
            }
            limits = {
                cpu     = "200m"
                memory  = "256Mi"
            }
        }
        prometheus = {
            monitor = {
                enabled     = true
                interval    = "15s"
            }
        }
    }
    le_annotations = [
    {
      name  = "ingress.annotations.cert-manager\\.io/cluster-issuer"
      value = "letsencrypt-production"
    }
  ]
}

resource "helm_release" "rabbitmq" {
  name              = "rabbitmq"
  repository        = "https://charts.bitnami.com/bitnami/"
  chart             = "rabbitmq"
  namespace         = "services"
  create_namespace  = true
  version           = "10.1.1"
  timeout = 900
  values            = [yamlencode(local.rabbit)]
  depends_on        = [var.dep]

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

resource "helm_release" "prometheus-rabbitmq-exporter" {
  name       = "prometheus-rabbitmq-exporter"
  namespace  = "prometheus"
  repository = "https://prometheus-community.github.io/helm-charts"
  chart      = "prometheus-rabbitmq-exporter"
  version    = "1.2.0"
  values     = [yamlencode(local.exporter)]
  depends_on = [helm_release.rabbitmq]
}
