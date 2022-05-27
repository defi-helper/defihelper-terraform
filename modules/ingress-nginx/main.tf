resource "kubernetes_namespace" "ingress-nginx-ns" {
  metadata {
    name = "ingress-nginx"
  }
}

locals {
  values = {
    controller = {
      nodeSelector = var.node_selector
      metrics = {
        enabled=true
        service = {
          annotations = {
            "prometheus.io/scrape" = "true"
            "prometheus.io/port" = "10254"
          }
        }
        serviceMonitor = {
          enabled = true
        }
      }
      podAnnotations = {
        "prometheus.io/scrape" = "true"
        "prometheus.io/port" = "10254"
      }
      "tolerations" =[{
        "effect" = "NoSchedule"
        "key" = "node-role.kubernetes.io/web"
        "operator" = "Equal"
        "value" = "true"
      }
      ]
      replicaCount = var.nginx_ingress_replicacount
      service = {
        loadBalancerIP = var.load_balancer_ip
        externalTrafficPolicy = "Local"
      }
      admissionWebhooks = {
        patch = {
          podAnnotations = {
            "sidecar.istio.io/inject" = "false"
          }
        }
      }
      "topologySpreadConstraints" = [{
        "maxSkew" = 1
        "topologyKey" = "topology.kubernetes.io/zone"
        "whenUnsatisfiable" = "DoNotSchedule"
      }
      ]
      "config" = {
        "log-format-escape-json" = "true"
        "log-format-upstream" = "{\"time\": \"$time_iso8601\", \"remote_addr\": \"$proxy_protocol_addr\", \"x_forward_for\": \"$proxy_add_x_forwarded_for\", \"request_id\": \"$req_id\", \"remote_user\": \"$remote_user\", \"bytes_sent\": $bytes_sent, \"request_time\": $request_time, \"status\": $status, \"vhost\": \"$host\", \"request_proto\": \"$server_protocol\", \"path\": \"$uri\", \"request_query\": \"$args\", \"request_length\": $request_length, \"duration\": $request_time,\"method\": \"$request_method\", \"http_referrer\": \"$http_referer\", \"http_user_agent\": \"$http_user_agent\" }"
        "enable-real-ip" = "true"
        "forwarded-for-header" = "CF-Connecting-IP"
      }
      resources = {
        "limits" = {
          "cpu" = "4000m"
          "memory" = var.nginx_ingress_memory_request
        }
        "requests" = {
          "cpu" = var.nginx_ingress_cpu_request
          "memory" = var.nginx_ingress_memory_request
        }
      }
      autoscaling = {
        "enabled" = "enable"
        "maxReplicas" = var.nginx_ingress_replicacount_max
        "minReplicas" = var.nginx_ingress_replicacount
        "targetCPUUtilizationPercentage" = 70
        "targetMemoryUtilizationPercentage" = 70
      }
    }
    defaultBackend = {
      enabled = true
      replicaCount = var.nginx_ingress_backend_replicacount
      nodeSelector = var.node_selector
      "tolerations" =[{
        "effect" = "NoSchedule"
        "key" = "node-role.kubernetes.io/web"
        "operator" = "Equal"
        "value" = "true"
      }
      ]
      resources = {
        "limits" = {
          "cpu" = "500m"
          "memory" = "30Mi"
        }
        "requests" = {
          "cpu" = "100m"
          "memory" = "30Mi"
        }
      }
    }
    tcp = {
      22 = "gitlab/gitlab-gitlab-shell:22"
    }
  }
}

resource "helm_release" "nginx-ingress" {
  name       = "nginx-ingress-controller"
  chart      = "${path.root}/modules/ingress-nginx/helm/ingress-nginx"
  namespace  = "ingress-nginx"
  create_namespace = true
  values = [yamlencode(local.values)]
  depends_on = [
    var.dep,
    kubernetes_namespace.ingress-nginx-ns,
  ]
}
