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
            name = "PostgresDefihelperConnLimitReached"
            rules = [{
              alert = "PostgresDefihelperConnLimit"
              expr = "(pooler_defihelper_tcp_connections + ${var.pg_defihelper_conn_limit}/100*10  >= ${var.pg_defihelper_conn_limit})"
              for = "0m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Postgres Defihelper connections limit reached"
                description = "Postgres Defihelper connections limit reached"
              }
            }]
          },
          {
            name = "PostgresScannerConnLimitReached"
            rules = [{
              alert = "PostgresScannerConnLimit"
              expr = "(pooler_scanner_tcp_connections + ${var.pg_scanner_user_conn_limit}/100*10  >= ${var.pg_scanner_user_conn_limit})"
              for = "0m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Postgres Scanner connections limit reached"
                description = "Postgres Scanner connections limit reached"
              }
            }]
          },
          {
            name = "PostgresAdaptersConnLimitReached"
            rules = [{
              alert = "PostgresAdaptersConnLimit"
              expr = "(pooler_adapters_tcp_connections + ${var.pg_adapters_user_conn_limit}/100*10  >= ${var.pg_adapters_user_conn_limit})"
              for = "0m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Postgres Adapters connections limit reached"
                description = "Postgres Adapters connections limit reached"
              }
            }]
          },
          {
            name = "PostgresOpenConnLimitReached"
            rules = [{
              alert = "PostgresOpenConnLimit"
              expr = "(pooler_ppen_tcp_connections + ${var.pg_open_user_conn_limit}/100*10  >= ${var.pg_open_user_conn_limit})"
              for = "0m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Postgres Open connections limit reached"
                description = "Postgres Open connections limit reached"
              }
            }]
          },
          {
            name = "PostgresBaConnLimitReached"
            rules = [{
              alert = "PostgresBaConnLimit"
              expr = "(pooler_ppen_tcp_connections + ${var.pg_ba_user_conn_limit}/100*10  >= ${var.pg_ba_user_conn_limit})"
              for = "0m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Postgres Ba connections limit reached"
                description = "Postgres Ba connections limit reached"
              }
            }]
          },
          {
            name = "RabbitmqTooManyMessagesInQueue"
            rules = [{
              alert = "RabbitmqTooManyMessagesInQueue"
              expr = "rabbitmq_queue_messages_ready > 1000"
              for = "2m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Rabbitmq too many messages in queue (instance {{ $labels.instance }})"
                description = "Queue is filling up (> 1000 msgs)\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
              }
            }]
          },
          {
            name = "KubernetesTooManyPods"
            rules = [{
              alert = "KubernetesTooManyPods"
              expr = "count(kube_pod_info) > 1000"
              for = "2m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Kubernetes too many pods"
                description = "Kubernetes too many pods VALUE = {{ $value }}"
              }
            }]
          },
          {
            name = "ManagedDatabaseHighCPUusage"
            rules = [{
              alert = "ManagedDatabaseHighCPUusage"
              expr = "cpu_idle < 30"
              for = "5m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Managed Database high CPU usage (service {{ $labels.resource_id }})"
                description = "Idle less than 30 percent\n  VALUE = {{ $value }}\n  RESOURCE = {{ $labels.resource_id }}\n  HOST = {{ $labels.host }}"
              }
            }]
          },
          {
            name = "ManagedDatabaseHighDiskUsage"
            rules = [{
              alert = "ManagedDatabaseHighDiskUsage"
              expr = "(disk_used_bytes*100) / disk_total_bytes > 85"
              for = "5m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Managed Database high Disk usage (service {{ $labels.resource_id }})"
                description = "Disk usage is more than 85 percent\n  VALUE = {{ $value }}\n  RESOURCE = {{ $labels.resource_id }}\n  HOST = {{ $labels.host }}"
              }
            }]
          },
          {
            name = "NginxHighHttp4xxErrorRate"
            rules = [{
              alert = "NginxHighHttp4xxErrorRate"
              expr = "sum(rate(nginx_ingress_controller_requests{status=~'^4..'}[1m])) by (ingress) / sum(rate(nginx_ingress_controller_requests[1m])) by (ingress) * 100 > 10"
              for = "1m"
              labels = {
                severity = "warning"
              }
              annotations = {
                summary = "Nginx high HTTP 4xx error rate (ingress - {{ $labels.ingress }})"
                description = "Too many HTTP requests with status 4xx (> 10%)\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
              }
            }]
          },
          {
            name = "NginxHighHttp[502,503,504]ErrorRate"
            rules = [{
              alert = "NginxHighHttp5xxErrorRate"
              expr = "sum(rate(nginx_ingress_controller_requests{status=~'^50[2-4]'}[1m])) by (ingress) / sum(rate(nginx_ingress_controller_requests[1m])) by (ingress) * 100 > 5"
              for = "1m"
              labels = {
                severity = "critical"
              }
              annotations = {
                summary = "Nginx high HTTP [502,503,504] error rate (ingress - {{ $labels.ingress }})"
                description = "Too many HTTP requests with status [502,503,504] (> 5%)\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
              }
            }]
          },
          {
            name = "NginxHighHttp5xxErrorRate"
            rules = [{
              alert = "NginxHighHttp5xxErrorRate"
              expr = "sum(rate(nginx_ingress_controller_requests{status=~'^5..'}[1m])) by (ingress) / sum(rate(nginx_ingress_controller_requests[1m])) by (ingress) * 100 > 5"
              for = "1m"
              labels = {
                severity = "warning"
              }
              annotations = {
                summary = "Nginx high HTTP 5xx error rate (ingress - {{ $labels.ingress }})"
                description = "Too many HTTP requests with status 5xx (> 5%)\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
              }
            }]
          },
          {
            name = "NginxLatencyHigh"
            rules = [{
              alert = "NginxLatencyHigh"
              expr = "histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[2m])) by (host, node)) > 3"
              for = "2m"
              labels = {
                severity = "warning"
              }
              annotations = {
                summary = "Nginx latency high (instance {{ $labels.instance }})"
                description = "Nginx p99 latency is higher than 3 seconds\n  VALUE = {{ $value }}\n  LABELS = {{ $labels }}"
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
        "auth.gitlab" = {
          enabled = true
          allow_sign_up = true
          client_id = var.grafana_gitlab_application_id
          client_secret = var.grafana_gitlab_secret
          scopes = "read_api"
          auth_url = "https://adcorn-prod.gitlab.yandexcloud.net/oauth/authorize"
          token_url = "https://adcorn-prod.gitlab.yandexcloud.net/oauth/token"
          api_url = "https://adcorn-prod.gitlab.yandexcloud.net/api/v4"
          allowed_groups = ""
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
        additionalScrapeConfigs = [
          {
            job_name = "yc-monitoring-postgres"
            metrics_path = "/monitoring/v2/prometheusMetrics"
            params = {
              folderId = [var.folder_id]
              service = ["managed-postgresql"]
            }
            bearer_token = var.monitoring_sa_api_key
            static_configs = [
              {
                targets = ["monitoring.api.cloud.yandex.net"]
                labels = {
                  folderId = var.folder_id
                  service = var.postgres_id
                }
              }
            ]
          },
          {
            job_name = "yc-monitoring-redis"
            metrics_path = "/monitoring/v2/prometheusMetrics"
            params = {
              folderId = [var.folder_id]
              service = ["managed-redis"]
            }
            bearer_token = var.monitoring_sa_api_key
            static_configs = [
              {
                targets = ["monitoring.api.cloud.yandex.net"]
                labels = {
                  folderId = var.folder_id
                  service = var.redis_id
                }
              }
            ]
          }
        ]
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
            as = "rabbitmq_watcher_queue_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"scanner_tasks_default\"} - rabbitmq_queue_consumers {queue=\"scanner_tasks_default\"} + 10"
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
            as = "rabbitmq_backend_queue_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"tasks_default\"} - rabbitmq_queue_consumers {queue=\"tasks_default\"} + 10"
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
            as = "rabbitmq_backend_history_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"tasks_metricHistory\"} - rabbitmq_queue_consumers {queue=\"tasks_metricHistory\"} + 10"
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
            as = "rabbitmq_backend_metrics_messages_ready"
          }
          metricsQuery = "rabbitmq_queue_messages {queue=\"tasks_metricCurrent\"} - rabbitmq_queue_consumers {queue=\"tasks_metricCurrent\"} + 10"
        },
      ]
    }
  }
  alertmanagerBot = {
    env = {
      "ALERTMANAGER_URL" = "http://kube-prometheus-stack-alertmanager:9093"
      "TELEGRAM_ADMIN" = var.telegram_bot_admins
      "TELEGRAM_TOKEN" = var.telegram_token
    }
    service = {
      main = {
        enabled = true
        primary = true
        ports = {
          http = {
            port = 8080
            targetPort = 8080
          }
        }
      }
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

/*
resource "helm_release" "alertmanager-bot" {
  name        = "alertmanager-bot"
  chart       = "${path.root}/modules/prometheus/helm/alertmanager-bot"
  namespace   = kubernetes_namespace.prometheus.metadata[0].name
  values      = [yamlencode(local.alertmanagerBot)]
  atomic      = true
  depends_on = [helm_release.kube-prometheus-stack]
}
*/

resource "helm_release" "alertmanager-notifier" {
  name        = "alertmanager-notifier"
  chart       = "${path.root}/modules/prometheus/helm/alertmanager-notifier"
  namespace   = kubernetes_namespace.prometheus.metadata[0].name
  values      = [yamlencode(local.alertmanagerNotifier)]
  atomic      = true
  depends_on = [helm_release.kube-prometheus-stack]
}
