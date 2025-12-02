// src/utils/format.ts
export const formatTime = (timestamp: number): string => {
  const date = new Date(timestamp)
  const now = Date.now()
  const diff = now - timestamp

  // 1 分钟内
  if (diff < 60 * 1000) return '刚刚'

  // 1 小时内
  if (diff < 3600 * 1000) return `${Math.floor(diff / 60000)} 分钟前`

  // 今天
  const today = new Date(now)
  const target = new Date(timestamp)
  if (
    today.getFullYear() === target.getFullYear() &&
    today.getMonth() === target.getMonth() &&
    today.getDate() === target.getDate()
  ) {
    return date.toLocaleTimeString('zh-CN', {
      hour: '2-digit',
      minute: '2-digit',
    })
  }

  // 今年
  if (today.getFullYear() === target.getFullYear()) {
    return date.toLocaleString('zh-CN', {
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit',
    })
  }

  // 更早
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  })
}

export function formatCount(num: number) {
  if (num >= 10000) return (num / 10000).toFixed(1).replace('.0', '') + 'w'
  if (num >= 1000) return (num / 1000).toFixed(1).replace('.0', '') + 'k'
  return num
}
