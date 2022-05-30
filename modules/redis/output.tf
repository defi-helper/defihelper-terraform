#output "status" {
#  value = yandex_mdb_redis_cluster.redis[0].status
#}
#output "status_regional" {
#  value = yandex_mdb_redis_cluster.redis_regional[0].status
#}
output "redis_password" {
  value = random_string.redis-password.result
}

output "redis_id" {
  value = yandex_mdb_redis_cluster.redis_regional[0].id
}
