const Discord = require('discord.js');

const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {formatPoints} = require('../util/format.js');
const {palete} = require('../appconfig.js');

var sprintf = require('sprintf-js').sprintf;

const responseSuccess = (league, message) => {
	var teams = league.teams;
	var teamMembers = teams.reduce((map, team_name) => {
		map[team_name] = [];
		return map;
	}, {});
	var teamPoints = teams.reduce((map, team_name) => {
		map[team_name] = 0;
		return map;
	}, {});
	var num_in_teams = 0;
	for (var i = 0; i < league.members.length; i++) {
		if (league.members[i].team) {
			teamMembers[league.members[i].team].push(league.members[i]);
			teamPoints[league.members[i].team] += league.members[i].points;
			num_in_teams++;
		}
	}

	var embed = new Discord.MessageEmbed().setColor(palete[1])
		.setTitle(`[League] ${league.league_name}`)
		.setDescription(
			`Start at: ${league.start_time ? new Date(league.start_time) : "TBA"}
			End at: ${league.end_time ? new Date(league.start_time) : "TBA"}
			Participants: ${num_in_teams}/${league.members.length} in teams
			Points Total: ${sprintf("%10.4g", Object.entries(teamPoints).map((keyValuePair, index) => { return keyValuePair[1]})
			.reduce((prev, cur) => {return prev + cur;}, 0))}`
		);

	teams.sort((a,b) => {
		return teamPoints[b] - teamPoints[a];
	});

	teams.forEach((team_name, index) => {
		teamMembers[team_name].sort((a, b) => {
			return b.points - a.points;
		});

		var value = `🔺 Total Points: ${formatPoints(teamPoints[team_name])}`;

		value += "\n\`\`\`" + sprintf("%8.8s\t%-20.20s\t%10.10s\t%4.4s", "Ranking", "Name", "Points", "Solved");

		for (var i = 0; i < teamMembers[team_name].length && i < 10; i++) {
			value += '\n';
			value += sprintf("%8d\t%-20s\t%10.4g\t%4d", i+1, 
				teamMembers[team_name][i].username.replace("%2A", "#"), 
				teamMembers[team_name][i].points, 
				teamMembers[team_name][i].problems_solved);
		}
		value += '\`\`\`';

		embed.addField(`${team_name}${index == 0 ? " 👑" : ""}${index == 1 ? " 👀" : ""}`, 
			value, false);
	})

	return embed;
}

module.exports = {
	name: 'info',
	description: 'Retrieve info about the league',
	args: 1,
	usage: '<league_name>',
	guildOnly: true,
	category: 'League',
	execute(message, args) {
		const {API_URL} = process.env;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/getInfo?league_name=${args[0]}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(res.data, message));
				},
				(err) => { 
					message.channel.send(responseFail(message, err.response.data.comment));
				}
			);
		}, (err) => { 
			message.channel.send(responseFail(message, err.response || `Seems like the server refuses to connect.`)); 
		});
	}
}