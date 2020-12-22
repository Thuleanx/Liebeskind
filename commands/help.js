const Discord = require('discord.js');
const {palete, prefix} = require('../appconfig.js');
const {responseFail} = require('../util/responseFail.js');
const {boolToEmote} = require('../util/emoji.js');

var sprintf = require('sprintf-js').sprintf;

module.exports = {
	name: 'help',
	description: 'List all of the commands, or info about a specific command',
	usage: '[commandName]',
	execute(message, args) {
		const {commands} = message.client;

		if (!args.length) {
			var embed = new Discord.MessageEmbed().setColor(palete[6])
				.setTitle(`How to use ${message.client.user.tag}`)
				.setThumbnail(`${process.env.API_URL}/img/jovo.png`)
				.setDescription(`Below are a list of userful commands. If you want to find out what the commands do specifically, use \`${prefix}help <commandName>\`.`);

			var category_commands = commands.reduce((map, command) => {
				var category = command.category;
				if (!category) category = "Misc";

				if (!(category in map))
					map[category] = [];
				map[category].push(command);

				return map;
			}, {});

			Object.entries(category_commands).forEach((keyValuePair, index) => {
				var category = keyValuePair[0];
				var command_list = keyValuePair[1];
				var embedValue = `\`\`\`markdown\n`;
				command_list.forEach((command) => {
					embedValue += `\t* ${sprintf("%-15.15s\t%.30s\n", prefix+command.name, command.description)}`;
				});
				embedValue += '\n\`\`\`';
				embed.addField(category + ' commands', embedValue, false);
			});
			
			message.channel.send(embed);
		} else {
			var command = message.client.commands.get(args[0]) || 
				message.client.commands.find(cmd => cmd.aliases && cmd.aliases.includes(args[0]));
			
			if (!command) { 
				message.channel.send(responseFail(message, `Command ${args[0]} not found`));
			} else {
				var embed = new Discord.MessageEmbed().setColor(palete[4])
					.setTitle(`How to use ${args[0]}`)
					.setDescription(command.description)
					.addField(`Name`, command.name, true)
					.addField(`Aliases`, command.aliases&&command.aliases.length?command.aliases.join(', ') : "None", true)
					.addField(`Category`, command.category, true)
					.addField(`Cooldown`, `${command.cooldown||"None"}`, true)
					.addField(`Parameters`, `\`\`\`markdown\n${prefix}${command.name} ${command.usage}\n\`\`\``, false)
					.addField(`Extra Restrictions:`, `Use in DMs: ${boolToEmote(!command.guildOnly)} Use by non-admin: ${boolToEmote(!command.adminOnly)} Use in Guilds: ${boolToEmote(!command.dmOnly)}`, true)
					.setFooter(`Note that all operation has an implicit cooldown of 3 seconds, to prevent excessive spaming.`);

				message.channel.send(embed);
			}
		}
	}
}