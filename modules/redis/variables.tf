variable "redis_name" {
  type = string
}
variable "vpc_id" {
  type = string
}
variable "location_subnets" {
  type = list(object({
    id   = string
    zone = string
  }))
}
variable "enable_replication" {
  description = ""
  default     = ""
}
