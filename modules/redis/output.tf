output "redis_password" {
  value = random_string.redis-password.result
}

output "redis_id" {
  value = yandex_mdb_redis_cluster.redis_regional.id
}
