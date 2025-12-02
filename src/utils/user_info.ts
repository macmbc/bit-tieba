export function getToken(): string {
  const session = localStorage.getItem('user_session')
  if (!session) throw new Error('未登录')
  const { token } = JSON.parse(session)
  return token
}

export function getUid(): number {
  const session = localStorage.getItem('user_session')
  if (!session) throw new Error('未登录')
  const { uid } = JSON.parse(session)
  return uid
}

export function getUsername(): string {
  const session = localStorage.getItem('user_session')
  if (!session) throw new Error('未登录')
  const { username } = JSON.parse(session)
  console.log(username)
  return username
}
