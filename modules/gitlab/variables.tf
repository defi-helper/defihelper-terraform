variable "cluster_domain" {
  type = string
}
variable "gitlab_certificate_secret_name" {
  type = string
}
variable "ssl_1iu_ru_crt" {
  type = string
}
variable "ssl_1iu_ru_key" {
  type = string
}
variable "gitlab_psql_host" {
  type = string
}
variable "pg_git_password" {
  type = string
}
variable "pg_git_name" {
  type = string
}
variable "gitlab_redis_host" {
  type = string
}
# variable "redis_password" {
#   type = string
# }
variable "gitlab_redis_port" {
  type = string
}
variable "gitlab_email_from" {
  type = string
}
variable "gitlab_smtp_address" {
  type = string
}
variable "gitlab_smtp_port" {
  type = string
}
variable "gitlab_psql_port" {
  type = string
}
variable "node_selector" {
  type = map(string)
}
variable "gitlabstorage-static-key" {
  type = string
}
variable "backups_cron_git" {
  type = string
}

variable "gitlab_smtp_user_name" {
  type = string
}
variable "gitlab_smtp_password" {
  type = string
}
variable "git-s3cfg" {
  type = string
}
variable "gitlab-backup-storage" {
  type = string
}
variable "gitlab-tmp-storage" {
  type = string
}
variable "version_gitlab" {
  type = string
}
variable "enable_gitlab" {
  type = string
}
