/*
module "crds" {
  source = "./modules/crds"
}
*/
resource "kubernetes_namespace" "prometheus" {
  metadata {
    name = "prometheus"
  }
}

resource "kubernetes_secret" "prometheus-basic-auth" {
  metadata {
    name = "prometheus-basic-auth"
    namespace = kubernetes_namespace.prometheus.metadata[0].name
  }
  data = {
    auth = var.prometheus_auth
  }
  type = "Opaque"
}

locals {
  # workaround for https://github.com/hashicorp/terraform/issues/22405
  ingress_json = {
    for name, config in var.configs:
      name => lookup(config, "ingress", false) != false ? jsonencode({
        enabled = true
        hosts = [config["ingress"]["domain"]]
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
          "nginx.ingress.kubernetes.io/auth-secret" = kubernetes_secret.prometheus-basic-auth.metadata[0].name
          "nginx.ingress.kubernetes.io/auth-realm" = "Authentication Required"
        }) : jsonencode({})))
      }) : jsonencode({})
  }
  ingress = {
    for name, json in local.ingress_json:
      name => jsondecode(json)
  }
  # end of workaround
  disabled_component = {
    enabled = false
  }
  values = {
    additionalPrometheusRulesMap = {
      rule-name = {
        groups = [{
          name = "HostOomKillDetected"
          rules = [{
            alert = "HostOomKillDetected"
            expr = "increase(node_vmstat_oom_kill[1m]) > 0"
            for = "0m"
            labels = {
              severity = "critical"
            }
            annotations = {
              summary = "Host OOM kill detected (instance {{ $labels.instance }})"
              description = "OOM kill detected\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
            }
          }]
        },
        {
          name = "KubernetesOomKillDetected"
          rules = [{
            alert = "KubernetesContainerOomKiller"
            expr = "(kube_pod_container_status_restarts_total - kube_pod_container_status_restarts_total offset 10m >= 1) and ignoring (reason) min_over_time(kube_pod_container_status_last_terminated_reason{reason=\"OOMKilled\"}[10m]) == 1"
            for = "0m"
            labels = {
              severity = "critical"
            }
            annotations = {
              summary = "Kubernetes container oom killer (instance {{ $labels.instance }})"
              description = "Container {{ $labels.container }} in pod {{ $labels.namespace }}/{{ $labels.pod }} has been OOMKilled {{ $value }} times in the last 10 minutes.\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
            }
          }]
        },
        {
          name = "RabbitmqTooManyMessagesInQueue"
          rules = [{
            alert = "RabbitmqTooManyMessagesInQueue"
            expr = "rabbitmq_queue_messages_ready > 500"
            for = "2m"
            labels = {
              severity = "critical"
            }
            annotations = {
              summary = "Rabbitmq too many messages in queue (instance {{ $labels.instance }})"
              description = "Queue is filling up (> 500 msgs)\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
            }
          }]
        }]
      }
    }
    alertmanager = {
      ingress = local.ingress["alertmanager"]
      enabled = true
      alertmanagerSpec = {
        nodeSelector = var.configs["alertmanager"].node_selector
        storage = {
          volumeClaimTemplate = {
            spec = {
              storageClassName = var.configs["alertmanager"].storage_class
              accessModes = [var.configs["prometheus"].storage_mode]
              resources = {
                requests = {
                  storage = var.configs["alertmanager"].storage_size
                }
              }
            }
          }
        }
      }
      config = {
        "global" = {
          "resolve_timeout" = "5m"
          "smtp_auth_identity" = var.alertmanager_email_from
          "smtp_auth_password" = var.alertmanager_smtp_password
          "smtp_auth_username" = var.alertmanager_email_from
          "smtp_from" = var.alertmanager_email_from
          "smtp_smarthost" = var.alertmanager_smtp_address
        }
        "receivers" = [
          {
          "email_configs" = [
            {
              "to" = var.alertmanager_email_to
              send_resolved = true
            }
          ]
          "name" = "all"
          },
          {
          "name" = "criticals"
          "email_configs" = [
            {
              "to" = var.alertmanager_email_to
              send_resolved = true
            }
          ]
          "webhook_configs" = [
            {
              url = "http://alertmanager-notifier:8899/alert"
            },
/*            {
              url = "http://alertmanager-bot:8080"
            }*/
          ]
          }
        ]
        "route" = {
          "group_by" = ["alertname"]
          "group_interval" = "10s"
          "group_wait" = "10s"
          "receiver" = "all"
          "repeat_interval" = "3h"
          "routes" = [
          {
            match = {
              severity = "warning"
            }
            receiver = "all"
            },
            {
            match = {
              severity = "critical"
            }
            receiver = "criticals"
          }
          ]
        }
      }
    }
    grafana = {
      ingress = local.ingress["grafana"]
      adminPassword = var.grafana_admin_password
      nodeSelector = var.configs["grafana"].node_selector
      additionalDataSources = [
      {
        name = "Loki"
        type = "loki"
        access = "proxy"
        url = "http://loki-stack.loki-stack.svc.cluster.local:3100"
        basicAuth = false
        jsonData = {
          tlsSkipVerify = true
          maxLines = 1000
        }
      }
      ]
      "grafana.ini" = {
        server = {
          root_url = "https://${local.ingress["grafana"].hosts[0]}"
        }
        paths = {
          data = "/var/lib/grafana/"
          logs = "/var/log/grafana"
          plugins = "/var/lib/grafana/plugins"
          provisioning = "/etc/grafana/provisioning"
        }
        analytics = {
          check_for_updates = true
        }
        log = {
          mode = "console"
        }
        grafana_net = {
          url = "https://grafana.net"
        }
      }
      persistence = {
        enabled = true
        size = "1Gi"
      }
    }
    kubeDns = local.disabled_component
    kubeScheduler = local.disabled_component
    kubeControllerManager = local.disabled_component
    kubeEtcd = local.disabled_component

    prometheusOperator = {
      nodeSelector = var.configs["operator"].node_selector
    }
    prometheus = {
      ingress = local.ingress["prometheus"]
      prometheusSpec = {
        nodeSelector = var.configs["prometheus"].node_selector
        serviceMonitorSelectorNilUsesHelmValues = false
        serviceMonitorSelector = {}
        serviceMonitorNamespaceSelector = {}
        retentionSize: "9GB"
        storageSpec = {
          volumeClaimTemplate = {
            spec = {
              storageClassName = var.configs["prometheus"].storage_class
              accessModes = [var.configs["prometheus"].storage_mode]
              resources = {
                requests = {
                  storage = var.configs["prometheus"].storage_size
                }
              }
            }
          }
        }
      }
    }
    kubeProxy = {
      enabled = false
    }
  }
  prometheusAdapter = {
    nodeSelector = var.configs["adapter"].node_selector
    prometheus = {
      url = "http://kube-prometheus-stack-prometheus.prometheus.svc.cluster.local"
      port = 9090
      path = ""
    }
    replicas = 1
    resources = {
      requests = {
        cpu = "100m"
        memory = "128Mi"
      }
      limits = {
        cpu = "100m"
        memory = "128Mi"
      }
    }
    rules = {
      default = false
      custom = [
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_converter_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"filestorage:converter\"} - rabbitmq_queue_consumers {queue=\"filestorage:converter\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_service_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"filestorage:service\"} - rabbitmq_queue_consumers {queue=\"filestorage:service\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_basic_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"filestorage:basic\"} - rabbitmq_queue_consumers {queue=\"filestorage:basic\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_uploader_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"filestorage:uploader\"} - rabbitmq_queue_consumers {queue=\"filestorage:uploader\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_webinar_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"filestorage:webinar\"} - rabbitmq_queue_consumers {queue=\"filestorage:webinar\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_ok_queue_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"ok:queue\"} - rabbitmq_queue_consumers {queue=\"ok:queue\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_billing_basic_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"billing:basic\"} - rabbitmq_queue_consumers {queue=\"billing:basic\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_at_mail_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"at:mail\"} - rabbitmq_queue_consumers {queue=\"at:mail\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^rabbitmq_queue_messages$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "rabbitmq_chat_queue_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"chat:queue\"} - rabbitmq_queue_consumers {queue=\"chat:queue\"} + 10"
        },
        {
          seriesQuery = "{__name__=~\"^pfp_fpm_active_processes$\"}"
          resources = {
            overrides = {
              namespace = {
                resource = "namespace"
              }
              pod = {
                resource = "pod"
              }
              service = {
                resource = "service"
              }
            }
          }
          name = {
            matches = "^(.*)"
            as = "php_fpm_active_processes_ok"
          }
          metricsQuery = "sum(phpfpm_active_processes{service=\"ok-backend-sm\"}) / sum(kube_deployment_status_replicas {deployment=\"ok-backend\"})"
        }
      ]
    }
  }
  alertmanagerNotifier = {
    env = {
      "TELEGRAM_TOKEN" = var.telegram_token
      "TELEGRAM_CHAT_ID" = var.telegram_chat_id
    }
  }
}

resource "helm_release" "kube-prometheus-stack" {
  name        = "kube-prometheus-stack"
  repository  = "https://prometheus-community.github.io/helm-charts"
  chart       = "kube-prometheus-stack"
  namespace   = kubernetes_namespace.prometheus.metadata[0].name
  version     = "18.1.0"
  values      = [yamlencode(local.values)]
  atomic      = true
#  depends_on  = [module.crds.req]
}

resource "helm_release" "prometheus-adapter" {
  name        = "prometheus-adapter"
  repository  = "https://prometheus-community.github.io/helm-charts"
  chart       = "prometheus-adapter"
  version     = "2.12.1"
  namespace   = kubernetes_namespace.prometheus.metadata[0].name
  values      = [yamlencode(local.prometheusAdapter)]
  atomic      = true
  depends_on = [helm_release.kube-prometheus-stack]
}

resource "helm_release" "alertmanager-notifier" {
  name        = "alertmanager-notifier"
  chart       = "${path.root}/modules/prometheus/helm/alertmanager-notifier"
  namespace   = kubernetes_namespace.prometheus.metadata[0].name
  values      = [yamlencode(local.alertmanagerNotifier)]
  atomic      = true
  depends_on = [helm_release.kube-prometheus-stack]
}