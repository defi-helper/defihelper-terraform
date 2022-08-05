#output "load_balancer_ip" {
#  value       = module.nginx-ingress.load_balancer_ip
#  description = "Nginx ingress load balancer ip"
#}
# output "elasticsearch_host" {
#   value       = module.elasticsearch.elasticsearch_host
#   description = "Elasticsearch cluster ingress host"
# }
# output "elasticsearch_user" {
#   value       = module.elasticsearch.elasticsearch_user
#   description = "Elasticsearch cluster user"
# }
# output "grafana_admin_password" {
#   value       = module.prometheus.grafana_admin_password
#   description = "Grafana admin user password"
# }
output "container_registry_id" {
  value       = module.registry.registry_id
  description = "Created container registry ID"
}
# output "prometheus_admin_password" {
#   value       = module.prometheus.prometheus_admin_password
#   description = "Prometheus basic-auth user password (username - prometheus)"
# }
output "redis_password" {
  value       = module.redis.redis_password
  description = "Redis cluster password"
}

output "s3_loki_static_access_key" {
  value = module.s3.s3_loki_static_access_key
}

output "s3_loki_static_secret_key" {
  value = module.s3.s3_loki_static_secret_key
  sensitive = true
}

output "s3_static_access_key" {
  value = module.s3.s3_static_access_key
}

output "s3_static_secret_key" {
  value = nonsensitive(module.s3.s3_static_secret_key)
}
