variable "cluster_domain" {
  type = string
}
variable "pgadmin4_admin_password" {
  type = string
}
variable "pgadmin4_domain" {
  type = string
}
variable "pgadmin4_admin_email" {
  type = string
}
variable "node_selector" {
  type = map(string)
}
