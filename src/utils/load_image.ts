import default1_avatar from '@/assets/avatar/default1.png'
import default2_avatar from '@/assets/avatar/default2.png'
import default3_avatar from '@/assets/avatar/default3.png'
import default4_avatar from '@/assets/avatar/default4.png'

// 创建头像数组
const avatars = [default1_avatar, default2_avatar, default3_avatar, default4_avatar]

export const getUserAvatar = (id: string): string => {
  const index = Number(id) % 4
  // 处理NaN情况（如果id不是有效数字）
  return avatars[index] || default1_avatar
}

import techLogo from '@/assets/forums/tech.png'
import gameLogo from '@/assets/forums/game.png'
import photoLogo from '@/assets/forums/photo.png'
// 名称 => 背景图 映射
const forumLogoMap: Record<string, string> = {
  科技讨论吧: techLogo,
  摄影交流吧: photoLogo,
  游戏组队吧: gameLogo,
}

export const getForumLogo = (avatar: string) => {
  if (!avatar) {
    return new URL('@/assets/forums/default.png', import.meta.url).href
  }

  // 例子： /res/forum_tech.jpg → tech
  const name = avatar.split('forum_')[1]?.split('.')[0]

  try {
    return new URL(`../assets/forums/${name}.png`, import.meta.url).href
  } catch (e) {
    return new URL('@/assets/forums/default.png', import.meta.url).href
  }
}
